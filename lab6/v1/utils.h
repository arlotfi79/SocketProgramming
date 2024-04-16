#ifndef UTILS_H
#define UTILS_H

struct forwardtab {
    unsigned long last_hop_address;
    unsigned short last_hop_port;
    unsigned long next_hop_address;
    unsigned short next_hop_port;
};

int find_available_entry(struct forwardtab tabentry[]);
void free_session_entry(struct forwardtab tabentry[], int index);
void parse_command(char buf[], char **args, char *passed_char) ;

#endif //UTILS_H
