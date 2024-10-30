#include "helper.c"

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

// determines if base station and sensor are in range of each other
bool inRangeBS(struct BaseStation* base, struct Sensor* sensor) {
    float dist = calulateDistance(base->x, base->y, sensor->x, sensor->y);
    if (dist <= sensor->range) return true;
    return false;
}

// determines if two sensors are in range of each other; assume sensor1 != sensor2
bool inRangeSS(struct Sensor* sensor1, struct Sensor* sensor2) {
    float dist = calulateDistance(sensor1->x, sensor1->y, sensor2->x, sensor2->y);
    if (dist <= sensor1->range && dist <= sensor2->range) return true;
    return false;
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
void closeSocket(int* server_socks, struct Sensor** sensors, int idx) {
    close(server_socks[idx]);
    server_socks[idx] = 0;
    free(sensors[idx]->id);
    free(sensors[idx]);
}

void handleUpdatePosition(int sockfd, struct BaseStation** bases, int num_bases, struct Sensor** sensors, char* msg, int idx) {
    // save sensor info
    struct Sensor* sensor = (struct Sensor*)malloc(sizeof(struct Sensor));
    saveSensor(sensor, msg);
    if (sensors[idx] != NULL) {
        free(sensors[idx]->id);
        free(sensors[idx]);
    }
    sensors[idx] = sensor;

    // printSensor(sensors[i]);

    char* list = calloc(MAX_LEN / 2, sizeof(char));
    // char list[MAX_LEN/2];
    char fStr[MAX_LEN];

    // get sensor info
    int dist = sensor->range;
    int x = sensor->x;
    int y = sensor->y;

    // populate the list of reachable
    int numReachable = 0;
    // find all reachable base stations
    for (int i = 0; i < num_bases; i++) {
        if (inRangeBS(bases[i], sensor)) {
            strcat(list, " ");
            strcat(list, bases[i]->id);
            strcat(list, " ");
            sprintf(fStr, "%f", bases[i]->x);
            strcat(list, fStr);
            strcat(list, " ");
            sprintf(fStr, "%f", bases[i]->y);
            strcat(list, fStr);
            numReachable++;
        }
    }

    // find all reachable sensors
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (idx != i && sensors[i] != NULL && inRangeSS(sensors[i], sensor)) {
            strcat(list, " ");
            strcat(list, bases[i]->id);
            strcat(list, " ");
            sprintf(fStr, "%f", bases[i]->x);
            strcat(list, fStr);
            strcat(list, " ");
            sprintf(fStr, "%f", bases[i]->y);
            strcat(list, fStr);
            numReachable++;
        }
    }

    // form message string in format: REACHABLE [NumReachable] [ReachableList]
    char message[MAX_LEN];
    snprintf(message, sizeof(message), "REACHABLE %d%s", numReachable, list);
    printf("sending message: %s\n", message);

    // finally, send back REACHABLE msg
    send(sockfd, message, strlen(message), 0); 

    // free space allocated
    free(list);
}

// handles WHERE BASE_ID/SENSOR_ID
// given id, find the base or sensor that matches the id and return its position
void getPosition(char* id, int numBases, struct BaseStation** bases, struct Sensor** sensors, float* x, float* y) {
    for (int i = 0; i < numBases; i++) {
        if (strcmp(bases[i]->id, id) == 0) {
            *x = bases[i]->x;
            *y = bases[i]->y;
        }
    }

    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (sensors[i] != NULL && strcmp(sensors[i]->id, id) == 0) {
            *x = sensors[i]->x;
            *y = sensors[i]->y;
        }
    }
}

// if the given ID is a sensor, return its socket index
// if not, return -1
int isSensor(char* id, struct Sensor** sensors) {
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (sensors[i] != NULL && strcmp(sensors[i]->id, id) == 0) {
            return i;
        }
    }
    return -1;
}

// given base ID, we return the base station object associated with it
// note that it is guaranteed that the ID is valid
struct BaseStation* getBaseObject(char* base_id, int numBases, struct BaseStation** bases) {
    for (int i = 0; i < numBases; i++) {
        if (strcmp(bases[i]->id, base_id) == 0) {
            return bases[i];
        }
    }
    return NULL;
}

// given a base, find all reachable bases (connected) or sensors (in range);
// return the complete list as returned_list and its size as list_size
void getReachableIDsForBase(char* base_id, int numBases, struct BaseStation** bases, int numConnected, struct Sensor** sensors, int* list_size, char*** returned_list) {
    struct BaseStation* base = getBaseObject(base_id, numBases, bases);

    // initialize list
    int numLinks = base->numLinks;
    char** list = calloc(numLinks + numConnected, sizeof(char*));

    // add all bases connected to base_id to the reachable list
    for (int i = 0; i < numLinks; i++) {
        list[i] = calloc(MAX_LEN, sizeof(char));
        strcpy(list[i], base->listOfLinks[i]);
    }

    // find sensors in range
    int idx = numLinks;
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (sensors[i] != NULL && inRangeBS(base, sensors[i])) {
            list[idx] = calloc(MAX_LEN, sizeof(char));
            strcpy(list[idx], sensors[i]->id);
            idx++;
        }
    }

    // return list and its size
    *list_size = idx;
    *returned_list = list;
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
        // printBase(base);
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

    // initialize socket array to -1 
    // for (int i=0; i< MAX_CONNECTIONS;i++){
    //     server_socks[i]= -1;
    // }

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
            int n = Readline(STDIN_FILENO, buffer, MAX_LEN);

            buffer[n-1] = '\0'; // replace the newline char with null terminator

            char request_type[MAX_LEN];
            getRequestType(buffer, request_type);
            if (strcmp(request_type, "QUIT") == 0) break;
        }
        else if (FD_ISSET(sockfd, &readfds)) { 
             // client is attempting to connect
            if (num_connected < MAX_CONNECTIONS) {
                // printf("connected!\n");
                int newsockfd = accept(sockfd, (struct sockaddr *) &cliaddr, &cli_addr_size);

                // find the empty socket pos
                for (int i = 0; i < MAX_CONNECTIONS; i++) {
                    if (server_socks[i] == 0) {
                        server_socks[i] = newsockfd;
                        break;
                    }
                }

                // FD_SET(STDIN_FILENO, &reads);
                FD_SET(newsockfd, &reads);
                num_connected++;
            }
            else {
                // Out of connections; close connection immediately
                int newsockfd = accept(sockfd, (struct sockaddr *) &cliaddr, &cli_addr_size);
                close(newsockfd);
            }
        }

        // receive messages from sensors (clients)
        for (int i = 0; i < MAX_CONNECTIONS; i++) {
            if (server_socks[i] > 0 && FD_ISSET(server_socks[i], &readfds)) {
                int n = recv(server_socks[i], buffer, MAX_LEN - 1, 0);
                buffer[n] = '\0'; // replace the newline char with null terminator

                if (n == 0) {
                    // Client closed connection
                    printf("closing on socket %d \n", i);
                    FD_CLR(server_socks[i], &reads);
                    closeSocket(server_socks, sensors, i);
                    num_connected--;
                }
                else {
                    // printf("message received: %s \n", buffer);
                    char request_type[MAX_LEN];
                    getRequestType(buffer, request_type);

                    if (strcmp(request_type, "UPDATEPOSITION") == 0) {
                        handleUpdatePosition(server_socks[i], bases, num_bases, sensors, buffer, i);
                    }
                    else if (strcmp(request_type, "WHERE") == 0) {
                        char* id;
                        float x;
                        float y;

                        // Use strtok to split the string by space
                        id = strtok(buffer, " ");
                        id = strtok(NULL, " \n\0");
                        // printf("id: %s\n", id);

                        // find the position of ID
                        // getPosition(id, num_bases, bases, num_connected, sensors, &x, &y);
                        getPosition(id, num_bases, bases, sensors, &x, &y);
                        // printf("found %s at (%f, %f)\n", id, x, y);

                        // return THERE message to client
                        char message[MAX_LEN];
                        snprintf(message, sizeof(message), "THERE %f %f", x, y); // THERE [NodeID] [XPosition] [YPosition]
                        send(server_socks[i], message, strlen(message), 0);
                    }
                    else if (strcmp(request_type, "DATAMESSAGE") == 0) {
                        printf("received: %s\n", buffer);
                        // first, we parse data message
                        strtok(buffer, " "); // read "DATAMESSAGE"
                        char* orig_id = strtok(NULL, " ");
                        char* next_id = strtok(NULL, " ");
                        char* dest_id = strtok(NULL, " ");
                        int list_size = atoi(strtok(NULL, " "));
                        
                        char** hop_list = calloc(list_size, sizeof(char*));
                        char* hop_list_str = strtok(NULL, " ");
                        // char* hop_list_str = calloc(25, sizeof(char));
                        // strcpy(hop_list_str, "THIS IS A TEST");
                        createHopListFromStr(hop_list_str, hop_list);

                        // printf("printing hop_list: \n");
                        // for (int i = 0; i < list_size; i++) {
                        //     printf("  %s\n", hop_list[i]);
                        // }
                        // printf("msg: %s\n", buffer);

                        // before doing anything, determine if this message's next destination is a sensor
                        // if sensor, send message directly to it
                        // int sensor_idx = isSensor("client1", sensors);
                        int sensor_idx = isSensor(next_id, sensors);
                        printf("sensor idx: %d\n", sensor_idx);
                        // if (sensor_idx >= 0) {
                        //     printf("sending: %s\n", buffer);
                        //     send(server_socks[sensor_idx], buffer, strlen(buffer), 0);
                        //     continue;
                        // }
                        

                        // run this loop while next id is a base station OR until dest reached
                        while (sensor_idx < 0) {
                            // message reached dest
                            if (strcmp(next_id, dest_id) == 0) {
                                printf("%s: Message from %s to %s successfully received.", dest_id, orig_id, dest_id);
                                break;
                            }

                            // while in this loop, we assume the message is sent from the base station, next_id
                            
                            // first, get all reachable base stations/sensors
                            
                        }

                        if (sensor_idx >= 0) {
                            printf("sending: %s\n", buffer);
                            send(server_socks[sensor_idx], buffer, strlen(buffer), 0);
                        }
        
                    }
                   else if (strcmp(request_type, "TEST") == 0) {
                        // FOR DEBUGGING PURPOSES ONLY
                        int list_size;
                        char** reachables;
                        getReachableIDsForBase("base_station_h", num_bases, bases, num_connected, sensors, &list_size, &reachables);

                        printf("reachable list: \n");
                        for (int i = 0; i < list_size; i++) {
                            printf("%s ", reachables[i]);
                        }
                        printf("\n");
                    }
                }
            }
        }
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

    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (sensors[i] != NULL) {
            free(sensors[i]->id);
            free(sensors[i]);
        }
    }
    free(sensors);

    free(buffer);
    fclose(file);

    return EXIT_SUCCESS;
}