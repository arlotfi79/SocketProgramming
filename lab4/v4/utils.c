#include <string.h>
#include <stdio.h>
#include "constants.h"

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


int is_command_allowed(char *command, char **args) {
    if (strcmp(command, "ls") == 0) {
        // Check argument constraints for "ls"
        // Check if the number of arguments exceeds the limit
        int args_count = 0;
        for (int j = 1; args[j] != NULL; j++) {
            if (strlen(args[j]) > MAX_COMMAND_ARG_LENGTH) {
                printf("Argument exceeds maximum length for ls command: %s\n", args[j]);
                return DISALLOW;
            }
            args_count++;
        }
        if (args_count > MAX_COMMAND_ARGS - 1) { // Subtract 1 for the command itself
            printf("Too many arguments for ls command: %s\n", args[0]);
            return DISALLOW;
        }
        return ALLOW;
    } else if (strcmp(command, "date") == 0) {
        return ALLOW;
    }
    return DISALLOW;
}