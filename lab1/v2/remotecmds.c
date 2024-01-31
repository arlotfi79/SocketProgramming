#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/stat.h> // for making the FIFO
#include <fcntl.h> // to use the read, open, ...

#include "utils.h"
#include "constants.h"

int main(void) {
    int k, fd, status;
    char buf[MAX_ARRAY_LENGTH];
    char *args[MAX_ARG_LENGTH], *tokens[MAX_ARG_LENGTH];
    ssize_t total_bytes_read = 0;

    unlink(FIFO_NAME); // to be able to make new fifo each time
    if (mkfifo(FIFO_NAME, ACCESS_LEVEL) == -1) {
        perror("Error creating FIFO");
        exit(EXIT_FAILURE);
    }

    while (1) {
        total_bytes_read = 0;
        // Flush the array after usage using memset
        memset(buf, '\0',sizeof(buf));

        fd = open(FIFO_NAME, O_RDONLY); // O_RDONLY: open for read-only
        if (fd == -1) {
            perror("Error opening FIFO for reading");
            exit(EXIT_FAILURE);
        }

        // Read command from FIFO
        total_bytes_read = read(fd, buf, sizeof(buf));
        if (total_bytes_read == -1) {
            perror("Error reading from FIFO");
            close(fd);
            exit(EXIT_FAILURE);
        }

        close(fd);

        parse_command(buf, tokens, "\n");

        for (int i = 0; tokens[i] != NULL; i++) {

            if (strlen(tokens[i]) > MAX_COMMAND_LENGTH)
                continue;

            // Flush the array after usage using memset
            memset(args, '\0', sizeof(args));

            parse_command(tokens[i], args, " ");

            k = fork();
            if (k == 0) {
                // child code
                if (execvp(args[0], args) == -1) {    // if execution failed, terminate child
                    perror("Error executing command");
                    exit(EXIT_FAILURE);
                }
            }
            else
                waitpid(k, &status, 0);        // block until child process terminates
        }

        // Flush the array after usage using memset
        memset(tokens, '\0', sizeof(tokens));
    }
}
