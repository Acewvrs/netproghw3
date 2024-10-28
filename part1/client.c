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

// send UPDATEPOSITION [SensorID] [SensorRange] [CurrentXPosition] [CurrentYPosition]
void sendUpdatePosition(int sockfd, char* id, float range, float x, float y) {
    char message[MAX_LEN];

    snprintf(message, sizeof(message), "UPDATEPOSITION %s %f %f %f", id, range, x, y);
    printf("sending %s\n", message);

    send(sockfd, message, strlen(message), 0);
}

void receiveReachable(int sockfd) {
    char message[MAX_LEN];
    int n = recv(sockfd, message, MAX_LEN - 1, 0);
    message[n] = '\0';

    printf("message: %s\n", message);

    int str_size = strlen(message) + 1; // to account for the trailing '\0', add 1
    int idx = 0;
    char* buffer = calloc(MAX_LEN, sizeof(char));
    int word_size = 0;

    // while (message[idx] != ' ') {
    //     buffer[idx] = message[idx];
    //     idx++;
    // }

    // int listLen = atoi(buffer);
    // printf("listLen: %d\n", listLen);

    for (int i = 0; i < str_size; i++) {
        if (message[i] == ' ' || message[i] == '\0') {
            buffer[word_size] = '\0';

            if (idx == 1) {
                printf("listLen: %d\n", atoi(buffer));
                // sensor->id = word;
                // word = calloc(MAX_LEN, sizeof(char));
            }
            else if (idx == 2) {
                // sensor->range = atof(word);
            }
            else if (idx == 3) {
                // sensor->x = atof(word);
            }
            else if (idx == 4) {
                // sensor->y = atof(word);
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
    free(buffer);
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

    sendUpdatePosition(sockfd, id, range, initX, initY);

    receiveReachable(sockfd);
    // while (true) {

    // }

    // send(sockfd, message, strlen(message), 0);
    // printf("Message sent to server: %s\n", message);

    return EXIT_SUCCESS;
}