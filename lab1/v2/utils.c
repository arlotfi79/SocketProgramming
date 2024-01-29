#include <string.h>

void parse_command(char buf[], char** args) {

    char *token = strtok(buf, " ");   // split the buffer into tokens
    int argIndex = 0;


    // tokenize and store all arguments in args[]
    while (token != NULL)
    {
        args[argIndex++] = token;
        token = strtok(NULL, " ");
    }
    args[argIndex] = NULL;             // last element should always be a null pointer
}
