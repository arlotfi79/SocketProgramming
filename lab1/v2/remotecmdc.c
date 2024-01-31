#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "constants.h"

int main() {
    int fd, len;
    char input[MAX_ARRAY_LENGTH];
    ssize_t bytes_written;

    fd = open(FIFO_NAME, O_WRONLY); // O_WRONLY: open for read-only
    if (fd == -1) {
        perror("Error opening FIFO for writing");
        exit(EXIT_FAILURE);
    }

    while (1) {
        printf("? ");
        fflush(stdout);

        // read command from stdin
        if (fgets(input, sizeof(input), stdin) == NULL) {
            perror("Error reading input");
            exit(EXIT_FAILURE);
        }

        len = strlen(input);

        // Check if input is empty (only return key pressed)
        if (len == 1) {
            continue;
        }

        // Check if input exceeds maximum length
        if (len > MAX_COMMAND_LENGTH) {
            printf("Error: Command exceeds maximum length\n");
            continue;
        }

        // Write command to FIFO
        bytes_written = write(fd, input, len);
        if (bytes_written == -1) {
            perror("Error writing to FIFO");
            close(fd);
            exit(EXIT_FAILURE);
        }

    }
}
