#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <ctype.h>
#include <arpa/inet.h>
#include "../../../lib/unp.h"
#include "helper.c"

// #include "unp.h" 

#define MAX_LEN 512

// includes both sensors and base stations
struct Reachable {
    char* id;
    float x;
    float y;
};

// send UPDATEPOSITION [SensorID] [SensorRange] [CurrentXPosition] [CurrentYPosition]
void sendUpdatePosition(int sockfd, char* id, float range, float x, float y) {
    char message[MAX_LEN];

    snprintf(message, sizeof(message), "UPDATEPOSITION %s %f %f %f", id, range, x, y);
    printf("sending %s\n", message);

    send(sockfd, message, strlen(message), 0);
}

// receive THERE command from server and save results in x, y
void receiveThere(int sockfd, char* buffer, float* x, float* y) {
    char message[MAX_LEN];
    int n = recv(sockfd, message, MAX_LEN - 1, 0);
    message[n] = '\0';

    // printf("received: %s\n", message);
    char* returned_msg;
    returned_msg = strtok(message, " ");
    returned_msg = strtok(NULL, " \n\0");
    *x = atof(returned_msg);
    returned_msg = strtok(NULL, " \n\0");
    *y = atof(returned_msg);
}

void sendData(int sockfd, char* buffer) {
    char* dest_id;
    char message[MAX_LEN];

    // printf("received: %s\n", message);
    char* returned_msg;
    returned_msg = strtok(message, " ");
    returned_msg = strtok(NULL, " \n\0");
    *dest_id = returned_msg;

    
}

// given ID of base station/sensor, get its position from server
void getPositionFromServer(int sockfd, char* id, float* x, float* y) {
    char message[MAX_LEN];
    snprintf(message, sizeof(message), "WHERE %s", id);
    
    send(sockfd, message, strlen(message), 0);
    receiveThere(sockfd, message, x, y);
}

// free all dynamically allocated space for items in reachable except the array (reachables)
void cleanReachables(const int numReachable, struct Reachable** reachables) {
    for (int i = 0; i < numReachable; i++) {
        free(reachables[i]->id);
        free(reachables[i]);
    }
}

void receiveReachable(int sockfd, int* numReachable, struct Reachable*** reachables_ptr) {
    char message[MAX_LEN];
    int n = recv(sockfd, message, MAX_LEN - 1, 0);
    message[n] = '\0';

    printf("message: %s\n", message);

    int str_size = strlen(message) + 1; // to account for the trailing '\0', add 1
    int idx = 0;
    char* buffer = calloc(MAX_LEN, sizeof(char));
    int word_size = 0;

    struct Reachable** reachables;
    struct Reachable* reachable;
    int listLen;
    for (int i = 0; i < str_size; i++) {
        if (message[i] == ' ' || message[i] == '\0') {
            buffer[word_size] = '\0';

            if (idx == 1) {
                listLen = atoi(buffer);
                reachables = calloc(listLen, sizeof(struct Reachable*));
                *numReachable = listLen; // save list size
            }
            else if (1 < idx && idx <= (listLen * 3) + 1 && idx % 3 == 2) {
                reachable = (struct Reachable*) malloc(sizeof(struct Reachable));
                reachable->id = buffer;
                buffer = calloc(MAX_LEN, sizeof(char));
            }
            else if (1 < idx && idx <= (listLen * 3) + 1 && idx % 3 == 0) {
                reachable->x = atof(buffer);
            }
            else if (1 < idx && idx <= (listLen * 3) + 1 && idx % 3 == 1) {
                reachable->y = atof(buffer);
                int reachable_idx = ((idx - 1) / 3) - 1; // converts 4, 7, 10... -> 0, 1, 2...
                reachables[reachable_idx] = reachable;
            }

            memset(buffer, '\0', sizeof(buffer)); // reset buffer
            word_size = 0;
            idx++;
        }
        else {
            buffer[word_size] = message[i];
            word_size++;
        }
    }
    *reachables_ptr = reachables; // save reachables list
    free(buffer);
}

void printReachableList(int size, struct Reachable** reachables) {
    printf("printing reachable list of size %d: \n", size);
    for (int i = 0; i < size; i++) {
        printf("  %s (%f, %f)\n", reachables[i]->id, reachables[i]->x, reachables[i]->y);
    }
}

int compareReachableIDs(const void* a, const void* b) {
    return strcmp(*(const char**)a, *(const char**)b);
}

// print IDs of reachable base stations and sensors in alphabetical order
void printReachableResult(char* sensor_id, int size, struct Reachable** reachables) {
    char** reachablesIDs = calloc(size, sizeof(char*));

    for (int i = 0; i < size; i++) {
        reachablesIDs[i] = reachables[i]->id;
    }

    qsort(reachablesIDs, size, sizeof(char*), compareReachableIDs);

    printf("%s: After reading REACHABLE message, I can see:", sensor_id);
    for (int i = 0; i < size; i++) {
        printf(" %s", reachablesIDs[i]);
    }
    printf("\n");
}

// given message that contains MOVE command from user, update x and y
void updatePosition(char* message, float* x, float* y) {
    int str_size = strlen(message) + 1; // add 1 to account for the trailing '\0'
    int idx = 0;
    char* word = calloc(MAX_LEN, sizeof(char));
    int word_size = 0;
    for (int i = 0; i < str_size; i++) {
        if (message[i] == ' ' || message[i] == '\0' || message[i] == '\n') {
            word[word_size] = '\0';

            printf("idx %d word: %s\n", idx, word);
            if (idx == 1) {
                *x = atof(word);
            }
            else if (idx == 2) {
                *y = atof(word);
            }

            memset(word, '\0', sizeof(word)); // reset buffer
            word_size = 0;
            idx++;
        }
        else {
            word[word_size] = message[i];
            word_size++;
        }
    }
    free(word);
}

int main(int argc, char ** argv ) {
    if (argc < 7) {
        fprintf(stderr, "ERROR: Invalid argument(s)\nUSAGE: ./client.out [control address] [control port] [SensorID] [SensorRange] [InitalXPosition] [InitialYPosition]\n");
        return EXIT_FAILURE;
    }

    char* control_address = argv[1];
    int port = atoi(argv[2]);
    char* id = argv[3];
    float range = atof(argv[4]);
    float x = atof(argv[5]);
    float y = atof(argv[6]);


    int sockfd;
    struct sockaddr_in servaddr;
    char buffer[MAX_LEN];

    // create socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        exit(EXIT_FAILURE);
    }

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port        = htons(port);

    // Convert IP address to binary and store in serveraddr
    if (inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr) <= 0) {
        perror("Invalid address or address not supported");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Connect to server
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("Connection failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("connected!\n");

    int numReachable;
    struct Reachable** reachables;

    sendUpdatePosition(sockfd, id, range, x, y);

    receiveReachable(sockfd, &numReachable, &reachables);
    // printReachableList(numReachable, reachables);
    printReachableResult(id, numReachable, reachables);
    
    // Initialize the file descriptor set
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds); 
    FD_SET(sockfd, &readfds);

    while (true) {
        int num_ready = select(FD_SETSIZE, &readfds, NULL, NULL, NULL);
        if (num_ready < 0) {
            perror("select error");
            exit(1);
        }

        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            // received user input
            int n = Readline(STDIN_FILENO, buffer, MAX_LEN);

            printf("read %d chars\n", n);
            buffer[n-1] = '\0'; // replace the newline char with null terminator

            char request_type[MAX_LEN];
            getRequestType(buffer, request_type);
            if (strcmp(request_type, "QUIT") == 0) break;
            else if (strcmp(request_type, "MOVE") == 0) {   // MOVE [NewXPosition] [NewYPosition]
                cleanReachables(numReachable, reachables);  // remove every item in reachables
                updatePosition(buffer, &x, &y);             
                // printf("updated pos: (%f, %f)\n", x, y);
                sendUpdatePosition(sockfd, id, range, x, y);

                // after notifying server of new positions, get new list of reachable base stations/sensors,
                receiveReachable(sockfd, &numReachable, &reachables);
                printReachableResult(id, numReachable, reachables);
            }
            else if (strcmp(request_type, "WHERE") == 0) { 
                // USER WON'T DIRECTLY CALL IT...
                float pos_x;
                float pos_y;
                getPositionFromServer(sockfd, "client1", &pos_x, &pos_y);
                printf("received client for pos: %f, %f\n", pos_x, pos_y);
            }
            else if (strcmp(request_type, "SENDDATA") == 0) {
                
            }
        }
        else if (FD_ISSET(sockfd, &readfds)) {
            
        }
    }

    // send(sockfd, message, strlen(message), 0);
    // printf("Message sent to server: %s\n", message);

    return EXIT_SUCCESS;
}