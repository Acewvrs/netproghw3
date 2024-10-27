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

int main(int argc, char ** argv ) {
    if (argc < 7) {
        fprintf(stderr, "ERROR: Invalid argument(s)\nUSAGE: ./client.out [control address] [control port] [SensorID] [SensorRange] [InitalXPosition] [InitialYPosition]\n");
        return EXIT_FAILURE;
    }

    char* control_address = argv[1];
    int port = atoi(argv[2]);
    char* id = argv[3];
    double range = atof(argv[4]);
    double initX = atof(argv[5]);
    double initY = atof(argv[6]);


    int sockfd;
    struct sockaddr_in servaddr;
    char buffer[MAX_LEN];
    char message[MAX_LEN] = "Hello, server!";

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

    while (true) {

    }
    
    // send(sockfd, message, strlen(message), 0);
    // printf("Message sent to server: %s\n", message);

    return EXIT_SUCCESS;
}