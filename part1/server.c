#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <ctype.h>
#include "../../../lib/unp.h"
// #include "unp.h" 

#define MAX_LEN 512
#define MAX_CONNECTIONS 10

struct BaseStation {
    char* id;
    float x;
    float y;
    int numLinks; // size of listOfLinks
    char** listOfLinks; // a space-separated list of base station IDs that this base station is connected to (bidirectional)
};

struct Sensor {
    char* id;
    float range;
    float x;
    float y;
};

void getRequestType(const char *input, char *request_type) {
    int i = 0;
    
    // Copy characters until we reach a space or the end of the input string
    while (input[i] != ' ' && input[i] != '\0') {
        request_type[i] = input[i];
        i++;
    }
    request_type[i] = '\0'; // Null-terminate the first word
}

void printBase(struct BaseStation* base) {
    printf("Printing Base Info: \n ID: %s\n Pos: (%f, %f)\n Num Links: %d\n", base->id, base->x, base->y, base->numLinks);

    printf(" Links: ");
    for (int i = 0; i < base->numLinks; i++) {
        printf("%s ", base->listOfLinks[i]);
    }
    printf("\n");
}

void saveBase(struct BaseStation* base, char* base_info) {
    int str_size = strlen(base_info);
    int word_idx = 0; // each word in a single line in base_file, separated by empty-char delimiter ' ' or '/n' (end of line)
    char* word = calloc(MAX_LEN, sizeof(char));
    int word_size = 0;
    for (int i = 0; i < str_size; i++) {
        if (base_info[i] == ' ' || base_info[i] == '\n') {
            word[word_size] = '\0';

            if (word_idx == 0) { // id
                base->id = word;
                word = calloc(MAX_LEN, sizeof(char));
            }
            else if (word_idx == 1) { // xPos
                base->x = atof(word);
            }
            else if (word_idx == 2) { // yPos
                base->y = atof(word);
            }
            else if (word_idx == 3) { // numLinks and initiate list of links
                base->numLinks = atoi(word);
                base->listOfLinks = calloc(atoi(word), sizeof(char*));
            }
            else { // add other bases to list of links
                base->listOfLinks[word_idx - 4] = word;
                word = calloc(MAX_LEN, sizeof(char));
            }

            // if (base_info[i] == ' ') word = calloc(MAX_LEN, sizeof(char)); // not the end of line; there's more words to process
            memset(word, '\0', sizeof(word)); // reset buffer
            word_size = 0;
            word_idx++;
        }
        else {
            word[word_size] = base_info[i];
            word_size++;
        }
    }
}

void printSensor(struct Sensor* sensor) {
    printf("Printing Sensor Info: \n ID: %s\n Range: %f\n Pos: (%f, %f)\n", sensor->id, sensor->range, sensor->x, sensor->y);
}

void saveSensor(struct Sensor* sensor, char* sensor_info) {
    int str_size = strlen(sensor_info) + 1; // to account for the trailing '\0', add 1
    int idx = 0;
    char* word = calloc(MAX_LEN, sizeof(char));
    int word_size = 0;
    for (int i = 0; i < str_size; i++) {
        if (sensor_info[i] == ' ' || sensor_info[i] == '\0') {
            word[word_size] = '\0';

            if (idx == 1) {
                sensor->id = word;
                word = calloc(MAX_LEN, sizeof(char));
            }
            else if (idx == 2) {
                sensor->range = atof(word);
            }
            else if (idx == 3) {
                sensor->x = atof(word);
            }
            else if (idx == 4) {
                sensor->y = atof(word);
            }

            memset(word, '\0', sizeof(word)); // reset buffer
            word_size = 0;
            idx++;
        }
        else {
            word[word_size] = sensor_info[i];
            word_size++;
        }
    }
    free(word);
}

// close an established socket and free space allocated for sensor
void close_socket(int* server_socks, struct Sensor** sensors, int idx) {
    close(server_socks[idx]);
    server_socks[idx] = 0;
    free(sensors[idx]->id);
    free(sensors[idx]);
}

int main(int argc, char ** argv ) {
    if (argc != 3) {
        fprintf(stderr, "ERROR: Invalid argument(s)\nUSAGE: ./control.out  [control port] [base station file]\n");
        return EXIT_FAILURE;
    }

    int port = atoi(argv[1]);
    char* base_file = argv[2];

    FILE *file = fopen(base_file, "r");
    if (file == NULL) {
        fprintf(stderr, "ERROR: INVALID FILE PATH\n");
        return EXIT_FAILURE;
    }

    // struct* bases = calloc()
    char* base_info = calloc(MAX_LEN, sizeof(char));
    int num_bases = 0;
    while(fgets(base_info, MAX_LEN, file)) {
        num_bases++;
    }

    // save all bases
    struct BaseStation** bases = calloc(num_bases, sizeof(struct BaseStation*));

    // reset file pointer
    fseek(file, 0, SEEK_SET);

    int base_idx = 0;
    while (fgets(base_info, MAX_LEN, file)) {
        struct BaseStation* base = (struct BaseStation*)malloc(sizeof(struct BaseStation));
        saveBase(base, base_info);
        bases[base_idx] = base;
        printBase(base);
        base_idx++;
    }

    int					sockfd;
    struct sockaddr_in	servaddr, cliaddr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // Set SO_REUSEADDR option
    int optval = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        perror("Error setting SO_REUSEADDR");
        return 1;
    }

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port        = htons(port);

    if (bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1) {
        perror("select error");
        exit(EXIT_FAILURE);
    }

    if (listen(sockfd, 3) == -1) {
        perror("listen error");
        exit(EXIT_FAILURE);
    }

    // saved sockets and info of each sensor (client)
    int* server_socks = calloc(MAX_CONNECTIONS, sizeof(int));
    struct Sensor** sensors = calloc(MAX_CONNECTIONS, sizeof(struct Sensor*));

    socklen_t cli_addr_size;

    fd_set readfds, reads;

    // Initialize the file descriptor set
    FD_ZERO(&reads); // zero out
    FD_SET(STDIN_FILENO, &reads); // detect user input for server
    FD_SET(sockfd, &reads);

    int num_connected = 0;
    char* buffer = calloc(MAX_LEN, sizeof(char));

    while (true) {
        readfds = reads;
        // Check if any of the connected servers have sent data
        int num_ready = select(FD_SETSIZE, &readfds, NULL, NULL, NULL);
        if (num_ready < 0) {
            perror("select error");
            exit(1);
        }

        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            // received user input
            Readline(STDIN_FILENO, buffer, MAX_LEN);

            buffer[strlen(buffer) - 1] = '\0'; // replace the newline char with null terminator
            if (strcmp(buffer, "QUIT") == 0) break;
            // printf("input: %s", buffer);
        }
        else if (FD_ISSET(sockfd, &readfds)) { 
             // client is attempting to connect
            if (num_connected < MAX_CONNECTIONS) {
                // printf("connected!\n");
                int newsockfd = accept(sockfd, (struct sockaddr *) &cliaddr, &cli_addr_size);
                // printf("Connected to port %d\n", port);
                // sprintf(msg, "Welcome to Guess the Word, please enter your username.\n");
                // send(newsockfd, msg, strlen(msg), 0);

                // find the empty socket pos
                for (int i = 0; i < MAX_CONNECTIONS; i++) {
                    if (server_socks[i] == 0) {
                        server_socks[i] = newsockfd;
                        break;
                    }
                }

                FD_SET(newsockfd, &reads);
                FD_SET(STDIN_FILENO, &reads);
                num_connected++;
            }
            else {
                // Out of connections; close connection immediately
                int newsockfd = accept(sockfd, (struct sockaddr *) &cliaddr, &cli_addr_size);
                close(newsockfd);
            }
        }

        for (int i = 0; i < MAX_CONNECTIONS; i++) {
            if (server_socks[i] > 0 && FD_ISSET(server_socks[i], &readfds)) {
                int n = recv(server_socks[i], buffer, MAX_LEN - 1, 0);
                if (n == 0) {
                    // Client closed connection
                    FD_CLR(server_socks[i], &reads);
                    // close_socket(server_socks, sensors, i);
                    num_connected--;
                }
                else {
                    // printf("received: %s \n", buffer);
                    char request_type[MAX_LEN];
                    getRequestType(buffer, request_type);

                    if (strcmp(request_type, "UPDATEPOSITION") == 0) {
                        struct Sensor* sensor = (struct Sensor*)malloc(sizeof(struct Sensor));
                        saveSensor(sensor, buffer);
                        sensors[i] = sensor;

                        printSensor(sensors[i]);
                    }
                }
            }
        }


        // for (int i = 0; i < 5; i++) {
        //     if (server_socks[i] > 0 && FD_ISSET(server_socks[i], &readfds)) {
        //         //  at least one server sent a message
        //         int n = recv(server_socks[i], buffer, MAX_LEN - 1, 0);
        //         if (n == 0) {
        //             // Client closed connection
        //             FD_CLR(server_socks[i], &reads);
        //             close_socket(server_socks, usernames, i);
        //             num_connected--;
        //         } 
        //         else if (n > 0) {
        //             remove_newline(buffer);
        //             buffer[stringSize(buffer)] = '\0';
                        
        //             if (usernames[i] == NULL) {
        //                 // user is setting the username
        //                 char* username = calloc(strlen(buffer), sizeof(char));
        //                 strcpy(username, buffer);

        //                 // check if the username is already in use
        //                 bool duplicate = false;
        //                 for (int j = 0; j < 5; j++) {
        //                     if (usernames[j] == NULL) continue;
                            
        //                     // compare the usernames without modifying the original input
        //                     char* username_in_use = calloc(strlen(usernames[j]), sizeof(char));
        //                     strcpy(username_in_use, usernames[j]);
        //                     if (strcmp(tolower_string(username_in_use), tolower_string(buffer)) == 0) {
        //                         duplicate = true;
        //                     }
        //                     free(username_in_use);
        //                 }
                       
        //                 if (!duplicate) {
        //                     usernames[i] = username;
        //                     sprintf(msg, "Let's start playing, %s\n", usernames[i]);
        //                     send(server_socks[i], msg, strlen(msg), 0);    
        //                     sprintf(msg, "There are %d player(s) playing. The secret word is %d letter(s).\n", num_connected, stringSize(hidden_word));
        //                 }
        //                 else {
        //                     sprintf(msg, "Username %s is already taken, please enter a different username\n", username);
        //                 }
        //                 send(server_socks[i], msg, strlen(msg), 0);                      
        //             }
        //             else {
        //                 // user is guessing the word
        //                 int guess_len = stringSize(buffer);
        //                 if (guess_len != stringSize(hidden_word)) {
        //                     sprintf(msg, "Invalid guess length. The secret word is %d letter(s).\n", stringSize(hidden_word));
        //                     send(server_socks[i], msg, strlen(msg), 0);
        //                 }
        //                 else {
        //                     // send guess result to every other client
        //                     int exact_match = strings_position_match(hidden_word, buffer);
        //                     int letters_match = strings_letters_match(hidden_word, buffer);

        //                     // check if the guess is correct
        //                     if (letters_match == stringSize(hidden_word)) {
        //                         sprintf(msg, "%s has correctly guessed the word %s", usernames[i], hidden_word);
        //                         guess_correct = true;
        //                     }
        //                     else {
        //                         sprintf(msg, "%s guessed %s: %d letter(s) were correct and %d letter(s) were correctly placed.\n", usernames[i], buffer, letters_match, exact_match);
        //                     }

        //                     for (int j = 0; j < 5; j++) {
        //                         if (usernames[j] == NULL) continue;
        //                         send(server_socks[j], msg, strlen(msg), 0);

        //                         if (guess_correct) {
        //                             // close the socket after a user guessed the secret word
        //                             FD_CLR(server_socks[i], &reads);
        //                             close_socket(server_socks, usernames, j);
        //                             num_connected--;
        //                         }
        //                     }
        //                 }
        //             }
        //         }
        //     }
        // }
    }

    // free dynamically allocated space
    for (int i = 0; i < num_bases; i++) {
        free(bases[i]->id);
        free(bases[i]->listOfLinks);
        free(bases[i]);
    }

    free(base_info);
    free(bases);

    free(server_socks);
    free(sensors);

    free(buffer);
    fclose(file);

    return EXIT_SUCCESS;
}