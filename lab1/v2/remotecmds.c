#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/stat.h> // for making the FIFO
#include <fcntl.h> // to use the read, open, ...

#include "utils.h"
#include "constants.h"

int main(void)
{
    int k, fd, status;
    char buf[MAX_COMMAND_LENGTH];
    char *args[MAX_ARG_LENGTH];
    ssize_t bytes_read, total_bytes_read = 0;

    unlink(FIFO_NAME); // to be able to make new fifo each time
    if (mkfifo(FIFO_NAME, ACCESS_LEVEL) == -1) {
        perror("Error creating FIFO");
        exit(EXIT_FAILURE);
    }

    while(1) {

        fd = open(FIFO_NAME, O_RDONLY); // O_RDONLY: open for read-only
        if (fd == -1) {
            perror("Error opening FIFO for reading");
            exit(EXIT_FAILURE);
        }

        // Read command from FIFO
        total_bytes_read = 0; // Reset total bytes read for each iteration
        while (total_bytes_read < MAX_COMMAND_LENGTH - 1)
        {
            // -1 is to keep space for the \0 which is going to be added
            bytes_read = read(fd, buf + total_bytes_read, MAX_COMMAND_LENGTH - 1 - total_bytes_read);
            if (bytes_read == -1)
            {
                perror("Error reading from FIFO");
                close(fd);
                exit(EXIT_FAILURE);
            }

            // No more data in FIFO
            else if (bytes_read == 0)
                break;

            // update read bytes
            total_bytes_read += bytes_read;

            // Command complete
            if (buf[total_bytes_read - 1] == '\n')
                break;
        }

        close(fd);

        buf[total_bytes_read-1] = '\0';

        // ignore if command exceeds max length
        if (total_bytes_read > MAX_COMMAND_LENGTH)
            continue;

        parse_command(buf, args);

        k = fork();
        if (k==0) {
            // child code
            if(execvp(args[0], args) == -1) {    // if execution failed, terminate child
                perror("Error executing command");
                exit(EXIT_FAILURE);
            }
        }
        else {
            // parent code
            waitpid(k, &status, 0);		// block until child process terminates
        }
    }
}
