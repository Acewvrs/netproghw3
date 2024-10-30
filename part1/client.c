#include "helper.c"

#define MAX_LEN 512

// includes both sensors and base stations
struct Reachable {
    char* id;
    float x;
    float y;
    float distToDest;
};

// check if the id of dest is in the list of reachables
bool isDestReachable(const int num_reachable, struct Reachable** reachables, char* dest_id) {
    for (int i = 0; i < num_reachable; i++) {
        if (strcmp(reachables[i]->id, dest_id) == 0) {
            return true;
        }
    }
    return false;
}

// send UPDATEPOSITION [SensorID] [SensorRange] [CurrentXPosition] [CurrentYPosition] to server
void sendUpdatePosition(int sockfd, char* id, float range, float x, float y) {
    char message[MAX_LEN];

    snprintf(message, sizeof(message), "UPDATEPOSITION %s %f %f %f", id, range, x, y);
    // printf("sending %s\n", message);

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

void sendData(int sockfd, char* origin_id, char* next_id, char* dest_id, int list_len, char* hop_list) {
    char message[MAX_LEN];
    snprintf(message, sizeof(message), "DATAMESSAGE %s %s %s %d %s", origin_id, next_id, dest_id, list_len, hop_list);
    send(sockfd, message, strlen(message), 0);
}

// given ID of base station/sensor, get its position from server
void getPositionFromServer(int sockfd, char* id, float* x, float* y) {
    char message[MAX_LEN];
    snprintf(message, sizeof(message), "WHERE %s", id);
    
    send(sockfd, message, strlen(message), 0);
    receiveThere(sockfd, message, x, y);
}

// free all dynamically allocated space for items in reachable except the array (reachables)
void cleanReachables(const int num_reachable, struct Reachable** reachables) {
    for (int i = 0; i < num_reachable; i++) {
        free(reachables[i]->id);
        free(reachables[i]);
    }
}

void receiveReachable(int sockfd, int* num_reachable, struct Reachable*** reachables_ptr) {
    char message[MAX_LEN];
    int n = recv(sockfd, message, MAX_LEN - 1, 0);
    message[n] = '\0';

    // printf("message: %s\n", message);

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
                *num_reachable = listLen; // save list size
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

// compare function for sorting IDs (string) in a list alphabetically
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

// calculate distance from reachable to dest
void saveDistanceToDest(int num_reachable, struct Reachable** reachables, float dest_x, float dest_y) {
    for (int i = 0; i < num_reachable; i++) {
        reachables[i]->distToDest = calulateDistance(reachables[i]->x, reachables[i]->y, dest_x, dest_y);
    }
}

int isFloatZero(float value) {
    const float epsilon = 1e-6; // Tolerance value
    return fabs(value) < epsilon;
}

// compare function for sorting IDs (string) in a list alphabetically
int compareReachableByDistance(const void* a, const void* b) {
    struct Reachable* r1 = *(struct Reachable**)a;
    struct Reachable* r2 = *(struct Reachable**)b;

    float diff = r1->distToDest - r2->distToDest;

    if (isFloatZero(diff)) { 
        // the distance to dest is the same
        // resolve ties by choosing the lexicographically smaller ID
        return strcmp(r1->id, r2->id); 
    }
    
    if (diff < 0) return -1;
    return 1;
}
 
// check if list contains given ID; if yes, we've visited the ID
bool isVisited(int list_size, char** hop_list, char* id) {
    for (int i = 0; i < list_size; i++) {
        if (strcmp(hop_list[i], id) == 0) {
            return true;
        }
    }
    return false;
}

// return false if there's no reachable base station/sensor to send next message to
// otherwise, return true and set next_id;
bool chooseNextID(int num_reachable, struct Reachable** reachables, int list_size, char** hop_list, char** next_id) {
    for (int i = 0; i < num_reachable; i++) {
        if (!isVisited(list_size, hop_list, reachables[i]->id)) {
            *next_id = reachables[i]->id;
            return true;
        }
    }
    return false;
}

// void makeHopListFromString(char* list_str) {
//     char word[MAX_LEN];

//     word = strtok(list_str, " ");
//     dest_id = strtok(NULL, " \n\0");
// }

void cleanList(int list_size, char** hop_list) {
    for (int i = 0; i < list_size; i++) {
        free(hop_list[i]);
    }
    free(hop_list);
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

    int num_reachable;
    struct Reachable** reachables;

    sendUpdatePosition(sockfd, id, range, x, y);

    receiveReachable(sockfd, &num_reachable, &reachables);
    // printReachableList(num_reachable, reachables);
    printReachableResult(id, num_reachable, reachables);
    
    // Initialize the file descriptor set
    fd_set readfds;
    // FD_ZERO(&readfds);
    // FD_SET(STDIN_FILENO, &readfds); 
    // FD_SET(sockfd, &readfds);

    while (true) {
        // reinitialize read file descriptors because select() resets it
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds); 
        FD_SET(sockfd, &readfds);

        int num_ready = select(FD_SETSIZE, &readfds, NULL, NULL, NULL);
        if (num_ready < 0) {
            perror("select error");
            exit(1);
        }

        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            // received user input
            int n = Readline(STDIN_FILENO, buffer, MAX_LEN);

            buffer[n-1] = '\0'; // replace the newline char with null terminator

            char request_type[MAX_LEN];
            getRequestType(buffer, request_type);
            if (strcmp(request_type, "QUIT") == 0) break;
            else if (strcmp(request_type, "MOVE") == 0) {   // MOVE [NewXPosition] [NewYPosition]
                cleanReachables(num_reachable, reachables);  // remove every item in reachables
                updatePosition(buffer, &x, &y);             
                // printf("updated pos: (%f, %f)\n", x, y);
                sendUpdatePosition(sockfd, id, range, x, y);

                // after notifying server of new positions, get new list of reachable base stations/sensors,
                receiveReachable(sockfd, &num_reachable, &reachables);
                printReachableResult(id, num_reachable, reachables);
            }
            else if (strcmp(request_type, "WHERE") == 0) { 
                // USER WON'T DIRECTLY CALL IT...
                float pos_x;
                float pos_y;
                getPositionFromServer(sockfd, "client1", &pos_x, &pos_y);
                printf("received client for pos: %f, %f\n", pos_x, pos_y);
            } 
            else if (strcmp(request_type, "SENDDATA") == 0) {
                // before sending data, get the latest list of reachables
                sendUpdatePosition(sockfd, id, range, x, y);
                receiveReachable(sockfd, &num_reachable, &reachables);

                // and create a list that contains the stations/sensors we've visited
                // currently, there's only one in the list (self)
                char** hop_list = calloc(1, sizeof(char*));
                hop_list[0] = calloc(MAX_LEN, sizeof(char));
                strcpy(hop_list[0], id);
                int list_size = 1;

                // then, get the ID of destination sensor/base station
                char* dest_id;
                dest_id = strtok(buffer, " ");
                dest_id = strtok(NULL, " \n\0");
                // printf("dest id: %s \n", dest_id);

                // first, check if dest is reachable
                if (isDestReachable(num_reachable, reachables, dest_id)) {
                    printf("%s: Sent a new message directly to %s\n", id, dest_id);
                    sendData(sockfd, id, dest_id, dest_id, list_size, id);
                    continue;
                }

                // if not reachable, then client sends WHERE to get position of dest_id
                float dest_x;
                float dest_y;
                getPositionFromServer(sockfd, dest_id, &dest_x, &dest_y);

                // then, it sorts all in-range sensors and base stations by distance to the destination
                saveDistanceToDest(num_reachable, reachables, dest_x, dest_y);
                qsort(reachables, num_reachable, sizeof(struct Reachable*), compareReachableByDistance);
                // printReachableList(num_reachable, reachables);

                // next, choose the first base station/sensor that we haven't visited
                char* next_id;
                if (!chooseNextID(num_reachable, reachables, list_size, hop_list, &next_id)) {
                    printf("%s: Message from %s to %s could not be delivered.", id, id, dest_id);
                    continue;
                }
                // printf("next id: %s\n", next_id);
                
                // finally, send data to that base station/sensor through control center
                printf("%s: Sent a new message bound for %s\n", id, dest_id);
                sendData(sockfd, id, next_id, dest_id, list_size, hop_list[0]);

                // clean memory of hop list we allocated
                cleanList(list_size, hop_list);
            }
        }
        else if (FD_ISSET(sockfd, &readfds)) {
            int n = recv(sockfd, buffer, MAX_LEN - 1, 0); // since server won't have newline in the end, use recv
            if (n == 0) {
                printf("Server closed the connection\n");
                close(sockfd);
                break;
            } 
            else if (n < 0) {
                perror("recv error");
            }
            buffer[n] = '\0';  // Null-terminate the received data

            // parse data message
            char* request_type = strtok(buffer, " ");
            char* orig_id = strtok(NULL, " ");
            strtok(NULL, " "); // next_id
            char* dest_id = strtok(NULL, " ");
            int list_size = atoi(strtok(NULL, " "));
            char** hop_list = calloc(list_size, sizeof(char*));
            char* hop_list_str = strtok(NULL, " ");
            createHopListFromStr(hop_list_str, hop_list);

            // message reached dest
            if (strcmp(id, dest_id) == 0) {
                printf("%s: Message from %s to %s successfully received.", id, orig_id, dest_id);
                continue;
            }

            // before sending data, get the latest list of reachables
            sendUpdatePosition(sockfd, id, range, x, y);
            receiveReachable(sockfd, &num_reachable, &reachables);

            // first, check if dest is reachable
            if (isDestReachable(num_reachable, reachables, dest_id)) {
                // add this sensor to the list of visited IDs
                strcat(hop_list_str, " ");
                strcat(hop_list_str, id); 
                list_size++;

                sendData(sockfd, id, dest_id, dest_id, list_size, id);
                continue;
            }

            // if not reachable, then client sends WHERE to get position of dest_id
            float dest_x;
            float dest_y;
            getPositionFromServer(sockfd, dest_id, &dest_x, &dest_y);

            // then, it sorts all in-range sensors and base stations by distance to the destination
            saveDistanceToDest(num_reachable, reachables, dest_x, dest_y);
            qsort(reachables, num_reachable, sizeof(struct Reachable*), compareReachableByDistance);
            // printReachableList(num_reachable, reachables);

            // next, choose the first base station/sensor that we haven't visited
            char* next_id;
            if (!chooseNextID(num_reachable, reachables, list_size, hop_list, &next_id)) {
                printf("%s: Message from %s to %s could not be delivered.", id, orig_id, dest_id);
                continue;
            }
            // printf("next id: %s\n", next_id);
            
            // add this sensor to the list of visited IDs
            strcat(hop_list_str, " ");
            strcat(hop_list_str, id); 
            list_size++;
            
            // finally, send data to that base station/sensor through control center
            printf("%s: Message from %s to %s being forwarded through %s\n", id, orig_id, dest_id, id);
            sendData(sockfd, id, next_id, dest_id, list_size, hop_list_str);

            // clean memory of hop list we allocated
            cleanList(list_size, hop_list);
        }
    }

    // send(sockfd, message, strlen(message), 0);
    // printf("Message sent to server: %s\n", message);

    return EXIT_SUCCESS;
}