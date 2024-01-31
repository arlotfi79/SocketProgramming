#include <string.h>

void parse_command(char buf[], char **args, char *passed_char) {

    char *token = strtok(buf, passed_char);   // split the buffer into tokens
    int argIndex = 0;


    // tokenize and store all arguments in args[]
    while (token != NULL) {
        args[argIndex++] = token;
        token = strtok(NULL, passed_char);
    }
    args[argIndex] = NULL;             // last element should always be a null pointer
}
