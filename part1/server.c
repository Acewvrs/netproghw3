#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <ctype.h>
#include "../../../lib/unp.h"
// #include "unp.h" 

#define MAX_LEN 512

struct BaseStation {
    char* id;
    float x;
    float y;
    int numLinks; // size of listOfLinks
    char** listOfLinks; // a space-separated list of base station IDs that this base station is connected to (bidirectional)
};

void printBase(struct BaseStation* base) {
    printf("Printing Base Info: \n ID: %s\n Pos: (%f, %f)\n Num Links: %d\n", base->id, base->x, base->y, base->numLinks);

    printf(" Links: ");
    for (int i = 0; i < base->numLinks; i++) {
        printf("%s ", base->listOfLinks[i]);
    }
    printf("\n");
}

void saveBase(struct BaseStation* base, char* base_info) {
    int word_idx = 0; // each word in a single line in base_file, separated by empty-char delimiter ' ' or '/n' (end of line)
    char* word = calloc(MAX_LEN, sizeof(char));
    int word_size = 0;
    for (int i = 0; i < strlen(base_info); i++) {
        if (base_info[i] == ' ' || base_info[i] == '\n') {
            word[word_size] = '\0';

            if (word_idx == 0) { // id
                base->id = word;
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
            }

            if (base_info[i] == ' ') word = calloc(MAX_LEN, sizeof(char)); // not the end of line; there's more words to process
            word_size = 0;
            word_idx++;
        }
        else {
            word[word_size] = base_info[i];
            word_size++;
        }
    }
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

    for (int i = 0; i < num_bases; i++) {
        free(bases[i]->id);
        free(bases[i]->listOfLinks);
        free(bases[i]);
    }

    free(base_info);
    free(bases);

    return EXIT_SUCCESS;
}