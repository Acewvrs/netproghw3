#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <ctype.h>
#include <arpa/inet.h>
#include "../../../lib/unp.h"
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

int main(int argc, char ** argv ) {
    if (argc < 7) {
        fprintf(stderr, "ERROR: Invalid argument(s)\nUSAGE: ./client.out [control address] [control port] [SensorID] [SensorRange] [InitalXPosition] [InitialYPosition]\n");
        return EXIT_FAILURE;
    }

    char* control_address = argv[1];
    int port = atoi(argv[2]);
    char* id = argv[3];
    float range = atof(argv[4]);
    float initX = atof(argv[5]);
    float initY = atof(argv[6]);


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

    sendUpdatePosition(sockfd, id, range, initX, initY);

    receiveReachable(sockfd, &numReachable, &reachables);
    // printReachableList(numReachable, reachables);
    printReachableResult(id, numReachable, reachables);
    
    // : After reading REACHABLE message, I can see: [SortedReachableList]

    // while (true) {

    // }

    // send(sockfd, message, strlen(message), 0);
    // printf("Message sent to server: %s\n", message);

    return EXIT_SUCCESS;
}