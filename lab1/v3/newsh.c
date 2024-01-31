// Concurrent server example: simple shell using fork() and execlp().

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include "utils.h"


int main(void) {
    int k;
    char buf[30];
    char *args[256];
    int status;
    int len;
    char hostname[256];

    if (gethostname(hostname, 256) != 0) {
        exit(1);
    }

    while (1) {

        // Print formatted prompt
        printf("%s:%d %%>", hostname, getpid());

        // read command from stdin
        fgets(buf, 30, stdin);
        len = strlen(buf);
        if (len == 1)                // case: only return key pressed
            continue;
        buf[len - 1] = '\0';            // case: command entered

        parse_command(buf, args, " ");

        k = fork();
        if (k == 0) {
            // child code
            char *envp[] = {NULL}; // Empty environment variables array
            char full_path[256];

            snprintf(full_path, sizeof(full_path), "/bin/%s", args[0]); // location of binaries in linux is /bin
            args[0] = full_path; // Update the first argument
            if (execve(args[0], args, envp) == -1)    // if execution failed, terminate child
                exit(1);
        } else {
            // parent code
            waitpid(k, &status, 0);        // block until child process terminates
        }
    }
}
