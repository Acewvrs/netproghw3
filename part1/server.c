#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <ctype.h>
#include "../../../lib/unp.h"
// #include "unp.h" 

int main(int argc, char ** argv ) {
    if (argc != 3) {
        fprintf(stderr, "ERROR: Invalid argument(s)\nUSAGE: ./control.out  [control port] [base station file]\n");
        return EXIT_FAILURE;
    }

    int port = argv[1];
    char* base_file = argv[2];

    return EXIT_SUCCESS;
}