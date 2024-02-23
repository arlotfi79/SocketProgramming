#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/stat.h> // for making the FIFO
#include <fcntl.h> // to use the read, open, ...

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>

#include "utils.h"
#include "constants.h"
#include "socket_utils.h"

// arg[0] = command, arg[1] = ip, arg[2] = port
int main(int argc, char *argv[]) {
    int k, status, len;
    char buf[MAX_ARRAY_LENGTH];
    char *args[MAX_ARG_LENGTH], *tokens[MAX_ARG_LENGTH];
    ssize_t total_bytes_read = 0;

    if (argc < S_ARG_COUNT) {
        fprintf(stderr, "Usage: %s <IPv4 address> <port number>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *const ip = argv[1];
    const char *const initial_port = argv[2];
    char port[MAX_PORT_NUM];


    int sockfd, new_fd, rv, numbytes, attempts = 0;
    struct addrinfo *servinfo, *p;
    struct sockaddr_in client_addr;
    char client_ip[INET_ADDRSTRLEN];

    socklen_t addr_len;


    do {
        snprintf(port, sizeof(port), "%d", atoi(initial_port) + attempts); // Increment port number

        if ((rv = build_address(ip, port, &servinfo) != 0)) {
            fprintf(stderr, "getaddrinfo server: %s\n", gai_strerror(rv));
            return EXIT_FAILURE;
        }

        // loop through all the results and bind to the first we can
        for (p = servinfo; p != NULL; p = p->ai_next) {
            if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == ERROR) {
                perror("Server: socket");
                continue;
            }


            // avoiding the "Address already in use" error message
            int yes = 1;
            if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == ERROR) {
                perror("setsockopt");
                exit(EXIT_FAILURE);
            }

            if (bind(sockfd, p->ai_addr, p->ai_addrlen) == ERROR) {
                perror("Server: bind");
                close(sockfd);
                continue;
            }

            break; // Bind successful
        }

        if (p == NULL) {
            // Bind successful, print the port number and exit
            fprintf(stderr, "Server: maximum number of attempts reached\n");
            freeaddrinfo(servinfo);
            return EXIT_FAILURE;
        }


    } while (++attempts < MAX_ATTEMPTS);


    printf("Server: successfully bound to port %s\n", port);
    freeaddrinfo(servinfo);

    if (listen(sockfd, MAX_CLIENTS) == ERROR) {
        perror("listen");
        return EXIT_FAILURE;
    }

    printf("Server: waiting for connections...\n");


    while (1) {
        printf("Server: waiting to recvfrom...\n");

        total_bytes_read = 0;
        // Flush the array after usage using memset
        memset(buf, '\0',sizeof(buf));

        // accept connection
        addr_len = sizeof client_addr;
        if ((new_fd = accept(sockfd, (struct sockaddr *) &client_addr, &addr_len)) == ERROR) {
            perror("accept");
            return EXIT_FAILURE;
        }

        // *** Source Address Filtering ***
        inet_ntop(AF_INET,
                  &(((struct sockaddr_in *) &client_addr)->sin_addr),
                  client_ip, sizeof client_ip);
        if (strncmp(client_ip, SOURCE_IP_PREFIX, strlen(SOURCE_IP_PREFIX)) != 0) {
            printf("Unauthorized access from %s\n", client_ip);
            close(new_fd);
            continue;
        }

        if ((numbytes = recv(new_fd, buf, sizeof(buf), 0)) == ERROR) {
            perror("recv");
            close(new_fd);
            exit(EXIT_FAILURE);
        }

        parse_command(buf, tokens, "\n");

        for (int i = 0; tokens[i] != NULL; i++) {

            if (strlen(tokens[i]) > MAX_COMMAND_LENGTH) {
                printf("Error in server: Command exceeds maximum length\n");
                continue;
            }

            // Flush the array after usage using memset
            memset(args, '\0', sizeof(args));

            parse_command(tokens[i], args, " ");

            // *** Command Restriction ***
            if (!is_command_allowed(args[0], args)) {
                printf("Unauthorized command: %s\n", args[0]);
                continue;
            }

            k = fork();
            if (k == 0) {
                // child code
                if (execvp(args[0], args) == ERROR) {    // if execution failed, terminate child
                    // Send response to client
                    char server_response[MAX_BUFFER_SIZE] = "command failed";
                    len = strlen(server_response);
                    server_response[len] = '\0';
                    if ((numbytes = send(new_fd, server_response, MAX_BUFFER_SIZE - 1, 0)) == ERROR) {
                        perror("server: send");
                        exit(EXIT_FAILURE);
                    }
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
