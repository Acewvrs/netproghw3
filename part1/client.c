#include "helper.c"

#define MAX_LEN 512

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

// given ID of base station/sensor, get its position from server
void getPositionFromServer(int sockfd, char* id, float* x, float* y) {
    char message[MAX_LEN];
    snprintf(message, sizeof(message), "WHERE %s", id);
    
    send(sockfd, message, strlen(message), 0);
    receiveThere(sockfd, message, x, y);
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

// print each item in reachable list in the format:
// ['base_station_a', 'base_station_b', 'base_station_d', 'base_station_e']
void printReachableList(int size, struct Reachable** reachables) {
    // printf("printing reachable list of size %d: \n", size);
    // ['base_station_a', 'base_station_b', 'base_station_d', 'base_station_e']
    printf("[");
    for (int i = 0; i < size; i++) {
        printf("'%s'", reachables[i]->id);
        if (i != size - 1) printf(", ");
    }
    printf("]");
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

    printf("%s: After reading REACHABLE message, I can see: ", sensor_id);
    // for (int i = 0; i < size; i++) {
    //     printf(" %s", reachablesIDs[i]);
    // }
    printReachableList(size, reachables);
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
    
    bool server_closed = false;

    while (true) {
        // reinitialize read file descriptors because select() resets it
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds); 
        if (!server_closed) FD_SET(sockfd, &readfds);

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
                printReachableResult(id, num_reachable, reachables);

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
                    printf("%s: Sent a new message directly to %s.\n", id, dest_id);
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
                char* next_id = calloc(MAX_LEN, sizeof(char));
                if (!chooseNextID(num_reachable, reachables, list_size, hop_list, next_id)) {
                    printf("%s: Message from %s to %s could not be delivered.", id, id, dest_id);
                    continue;
                }
                // printf("next id: %s\n", next_id);
                
                // finally, send data to that base station/sensor through control center
                printf("%s: Sent a new message bound for %s.\n", id, dest_id);
                sendData(sockfd, id, next_id, dest_id, list_size, hop_list[0]);

                // clean memory of hop list we allocated
                cleanHopList(list_size, hop_list);
            }
            else if (strcmp(request_type, "TEST") == 0) {
                // FOR DEBUGGING PURPOSES ONLY
                send(sockfd, buffer, sizeof(buffer), 0);
            }
        }
        else if (!server_closed && FD_ISSET(sockfd, &readfds)) {
            int n = recv(sockfd, buffer, MAX_LEN - 1, 0); // since server won't have newline in the end, use recv
            if (n == 0) {
                // printf("Server closed the connection\n");
                close(sockfd);
                server_closed = true;
                // break;
                continue;
            } 
            else if (n < 0) {
                perror("recv error");
            }
            buffer[n] = '\0';  // Null-terminate the received data
            printf("received: %s\n", buffer);

            // parse data message
            char* request_type = strtok(buffer, " ");
            char* orig_id = strtok(NULL, " ");
            strtok(NULL, " "); // next_id
            char* dest_id = strtok(NULL, " ");
            int list_size = atoi(strtok(NULL, " "));

            char** hop_list = calloc(list_size, sizeof(char*));
            for (int i = 0; i < list_size; i++) {
                hop_list[i] = calloc(MAX_LEN, sizeof(char));
            }
            char* hop_list_str = strtok(NULL, "\0\n"); // end of message
            
            createHopListFromStr(list_size, hop_list_str, hop_list);
            // printf("size: %d, list: %s\n", list_size, hop_list_str);

            // message reached dest
            if (strcmp(id, dest_id) == 0) {
                printf("%s: Message from %s to %s successfully received.\n", id, orig_id, dest_id);
                continue;
            }

            // before sending data, get the latest list of reachables
            sendUpdatePosition(sockfd, id, range, x, y);
            receiveReachable(sockfd, &num_reachable, &reachables);

            // first, check if dest is reachable
            if (isDestReachable(num_reachable, reachables, dest_id)) {
                // add this sensor to the list of visited IDs
                addToHopList(hop_list_str, id);
                list_size++;

                printf("%s: Message from %s to %s being forwarded through %s\n", id, orig_id, dest_id, id);
                printReachableResult(id, num_reachable, reachables);
                sendData(sockfd, orig_id, dest_id, dest_id, list_size, hop_list_str);
                continue;
            }
            printReachableResult(id, num_reachable, reachables);

            // if not reachable, then client sends WHERE to get position of dest_id
            float dest_x;
            float dest_y;
            getPositionFromServer(sockfd, dest_id, &dest_x, &dest_y);

            // then, it sorts all in-range sensors and base stations by distance to the destination
            saveDistanceToDest(num_reachable, reachables, dest_x, dest_y);
            qsort(reachables, num_reachable, sizeof(struct Reachable*), compareReachableByDistance);
            // printReachableList(num_reachable, reachables);

            // next, choose the first base station/sensor that we haven't visited
            char* next_id = calloc(MAX_LEN, sizeof(char));
            if (!chooseNextID(num_reachable, reachables, list_size, hop_list, next_id)) {
                printf("%s: Message from %s to %s could not be delivered.", id, orig_id, dest_id);
                continue;
            }
            // printf("next id: %s\n", next_id);
            
            // add this sensor to the list of visited IDs
            // strcat(hop_list_str, " ");
            // strcat(hop_list_str, id); 
            addToHopList(hop_list_str, id);
            list_size++;
            
            // finally, send data to that base station/sensor through control center
            printf("%s: Message from %s to %s being forwarded through %s\n", id, orig_id, dest_id, id);
            sendData(sockfd, orig_id, next_id, dest_id, list_size, hop_list_str);

            // clean memory of hop list we allocated
            cleanHopList(list_size, hop_list);
        }
    }

    // send(sockfd, message, strlen(message), 0);
    // printf("Message sent to server: %s\n", message);

    return EXIT_SUCCESS;
}