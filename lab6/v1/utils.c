#include <string.h>
#include "utils.h"
#include "constants.h"

int find_available_entry(struct forwardtab tabentry[]) {
    int tunnel_index;
    for (tunnel_index = 0; tunnel_index < MAX_CLIENTS; tunnel_index++) {
        if (tabentry[tunnel_index].last_hop_address == 0) {
            break;
        }
    }
    return (tunnel_index < MAX_CLIENTS) ? tunnel_index : -1;
}

void free_session_entry(struct forwardtab tabentry[], int index) {
    tabentry[index].last_hop_address = 0;
    tabentry[index].last_hop_port = 0;
    tabentry[index].next_hop_address = 0;
    tabentry[index].next_hop_port = 0;
}

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

