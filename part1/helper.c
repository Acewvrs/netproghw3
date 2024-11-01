// FOR FUNCTIONS AND LIBRARIES USED IN BOTH SERVER.C AND CLIENT.C
// #include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <ctype.h>
#include <math.h>
#include <arpa/inet.h>
// #include "../../../lib/unp.h"
#include "unp.h" 

#define MAX_LEN 512
// includes both sensors and base stations
struct Reachable {
    char* id;
    float x;
    float y;
    float distToDest;
};

void cleanHopList(int list_size, char** hop_list) {
    for (int i = 0; i < list_size; i++) {
        free(hop_list[i]);
    }
    free(hop_list);
}

void sendData(int sockfd, char* origin_id, char* next_id, char* dest_id, int list_len, char* hop_list) {
    char message[MAX_LEN];
    snprintf(message, sizeof(message), "DATAMESSAGE %s %s %s %d %s", origin_id, next_id, dest_id, list_len, hop_list);
    send(sockfd, message, strlen(message), 0);
}

// add ID we've visited to hop list
void addToHopList(char* hop_list_str, char* id) {
    strcat(hop_list_str, " ");
    strcat(hop_list_str, id); 
}

// free all dynamically allocated space for items in reachable except the array (reachables)
// note this does not free the array itself
void cleanReachables(const int num_reachable, struct Reachable** reachables) {
    for (int i = 0; i < num_reachable; i++) {
        free(reachables[i]->id);
        free(reachables[i]);
    }
}

int isFloatZero(float value) {
    const float epsilon = 1e-6; // Tolerance value
    return fabs(value) < epsilon;
}

// compare function for sorting IDs (string) in a list alphabetically (used for qsort)
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
bool chooseNextID(int num_reachable, struct Reachable** reachables, int list_size, char** hop_list, char* next_id) {
    for (int i = 0; i < num_reachable; i++) {
        if (!isVisited(list_size, hop_list, reachables[i]->id)) {
            // *next_id = reachables[i]->id;
            strcpy(next_id, reachables[i]->id);
            return true;
        }
    }
    return false;
}

// initialize a list of IDs from a string where items are separated by commas
// e.g. "client1 client2 client3" -> ["client1", "client2", "client3"]
// keeps the original string intact
void createHopListFromStr(int list_size, char* hop_list_str, char** hop_list) {
    char list_str_copy[MAX_LEN];
    strcpy(list_str_copy, hop_list_str);

    char* id = strtok(list_str_copy, " \0\n");
    for (int i = 0; i < list_size; i++) {
        strcpy(hop_list[i], id);
        id = strtok(NULL, " \0\n");
    }
}

// reads the first word received from client to figure msg type
void getRequestType(const char *input, char *request_type) {
    int i = 0;
    
    // Copy characters until we reach a space or the end of the input string
    while (input[i] != ' ' && input[i] != '\0' && input[i] != '\n') {
        request_type[i] = input[i];
        i++;
    }
    request_type[i] = '\0'; // Null-terminate the first word
}

float calulateDistance(float x1, float y1, float x2, float y2) {
    float x_dist = x1 - x2;
    float y_dist = y1 - y2;
    return sqrt(x_dist * x_dist + y_dist * y_dist);
}