#ifndef PARSE_COMMAND_H
#define PARSE_COMMAND_H

void parse_command(char buf[], char **args, char *passed_char);
int is_command_allowed(char *command, char **args);

#endif