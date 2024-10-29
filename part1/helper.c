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
#include "../../../lib/unp.h"
// #include "unp.h" 

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