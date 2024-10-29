// FOR FUNCTIONS USED IN BOTH SERVER.C AND CLIENT.C

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