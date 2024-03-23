#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "constants.h"
#include "utils.h"
#include "socket_utils.h"

struct forwardtab tabentry[MAX_CLIENTS] = {0};
void child_process(int control_conn);


int main(int argc, char *argv[]) {
    if (argc < S_ARG_COUNT) {
        fprintf(stderr, "Usage: %s <IPv4 address> <port number> <secret key>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *const ip = argv[1];
    const char *const port = argv[2];
    const char *const secret_key = argv[3];
    unsigned long dest_addr;
    unsigned long src_addr;
    unsigned short dest_port;


    int control_sock, control_conn, rv;
    struct addrinfo *servinfo, *p;
    struct sockaddr_in tunnelc;
    socklen_t addr_len;

    char buffer[1024];



    if ((rv = build_address(ip, port, SOCK_STREAM, &servinfo) != 0)) {
        fprintf(stderr, "getaddrinfo server: %s\n", gai_strerror(rv));
        return EXIT_FAILURE;
    }

    // loop through all the results and bind to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((control_sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == ERROR) {
            perror("Tunnel: socket");
            continue;
        }


        // avoiding the "Address already in use" error message
        int yes = 1;
        if (setsockopt(control_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == ERROR) {
            perror("setsockopt");
            exit(EXIT_FAILURE);
        }

        if (bind(control_sock, p->ai_addr, p->ai_addrlen) == ERROR) {
            perror("Tunnel: bind");
            close(control_sock);
            continue;
        }

        break; // Bind successful
    }

    if (p == NULL) {
        // Bind successful, print the port number and exit
        fprintf(stderr, "Server: Bind failed, try again with a different port!!\n");
        freeaddrinfo(servinfo);
        return EXIT_FAILURE;
    }


    printf("Server: successfully bound to port %s\n", port);
    freeaddrinfo(servinfo);

    if (listen(control_sock, MAX_CLIENTS) == ERROR) {
        perror("listen");
        return EXIT_FAILURE;
    }

    printf("Tunnel: waiting for connections...\n");

    while (1) {
        // accept connection
        addr_len = sizeof tunnelc;
        if ((control_conn = accept(control_sock, (struct sockaddr *) &tunnelc, &addr_len)) == ERROR) {
            perror("Tunnel: error accepting connection!");
            return EXIT_FAILURE;
        }


        // Fork child process to handle packet forwarding
        pid_t pid = fork();
        if (pid < 0) {
            perror("Error forking process");
            close(control_conn);
            continue;
        } else if (pid == 0) { // Child process code
            close(control_sock); // Close control socket in child process

            // Handle the child process tasks
            struct addrinfo *servinfo, *p;
            int data_sock, data_conn, attempts = 0;
            struct addrinfo *data_addr;

            char data_port[MAX_PORT_NUM];

            // creating first socket for communication between client and the tunnel
            do {
                snprintf(data_port, sizeof(data_port), "%d", atoi("60000") + attempts); // Increment port number

                if ((rv = build_address(ip, data_port, SOCK_DGRAM, &data_addr) != 0)) {
                    fprintf(stderr, "getaddrinfo server: %s\n", gai_strerror(rv));
                    return EXIT_FAILURE;
                }

                // loop through all the results and bind to the first we can
                for (p = data_addr; p != NULL; p = p->ai_next) {
                    if ((data_sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == ERROR) {
                        perror("Server: socket");
                        continue;
                    }


                    // avoiding the "Address already in use" error message
                    int yes = 1;
                    if (setsockopt(data_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == ERROR) {
                        perror("setsockopt");
                        exit(EXIT_FAILURE);
                    }

                    if (bind(data_sock, p->ai_addr, p->ai_addrlen) == ERROR) {
                        perror("Server: bind");
                        close(data_sock);
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

            printf("Server: successfully bound to port %s\n", data_port);
            freeaddrinfo(data_addr);

            // Inform client about the data port
            if (write(control_conn, &data_port, sizeof(data_port)) != sizeof(data_port)) {
                perror("Error writing data port to client");
                close(control_conn);
                close(data_sock);
                exit(EXIT_FAILURE);
            }

            // TODO: next steps after sending the data port to the client




            // Terminate child process
            exit(EXIT_SUCCESS);
        } else { // Parent process code

            // Read the first byte from the client
            if (read(control_conn, buffer, 1) < 0) {
                perror("Error reading first byte from control connection");
                close(control_conn);
                continue;
            }

            // Check if it's a new connection request
            if (buffer[0] != 'c') {
                printf("Invalid request: %c\n", buffer[0]);
                close(control_conn);
                continue;
            }

            // Read the secret key from the client
            if (read(control_conn, buffer, 8) < 0) {
                perror("Error reading secret key from control connection");
                close(control_conn);
                continue;
            }

            // Check if the secret key matches
            if (strncmp(buffer, secret_key, 8) != 0) {
                printf("Invalid secret key\n");
                close(control_conn);
                continue;
            }

            // Read destination IPv4 address from the client
            if (read(control_conn, buffer, 4) < 0) {
                perror("Error reading destination address from control connection");
                close(control_conn);
                continue;
            }

            dest_addr = *(unsigned long *)buffer;

            // Read destination port number from the client
            if (read(control_conn, buffer, 2) < 0) {
                perror("Error reading destination port from control connection");
                close(control_conn);
                continue;
            }

            dest_port = *(unsigned short *)buffer;

            // Read source IPv4 address from the client
            if (read(control_conn, buffer, 4) < 0) {
                perror("Error reading source address from control connection");
                close(control_conn);
                continue;
            }

            src_addr = *(unsigned long *)buffer;

            // Find available entry in tabentry[]
            int tunnel_index = find_available_entry(tabentry);

            if (tunnel_index != -1) {
                printf("Available entry found at index %d\n", tunnel_index);
            } else {
                printf("Maximum number of forwarding sessions reached\n");
                close(control_conn);
                continue;
            }

            // Update tabentry[] with client request details
            tabentry[tunnel_index].srcaddress = src_addr;
            tabentry[tunnel_index].srcport = src_addr;
            tabentry[tunnel_index].dstaddress = dest_addr;
            tabentry[tunnel_index].dstport = dest_port;

            // Close control connection in parent process
            close(control_conn);
            printf("New tunneling session established (tunnelindex: %d)\n", tunnel_index);
        }
    }

    // Close control socket
    close(control_sock);

    return 0;
}