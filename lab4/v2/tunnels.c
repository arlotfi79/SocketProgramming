#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdbool.h>

#include "constants.h"
#include "utils.h"
#include "socket_utils.h"

struct forwardtab tabentry[MAX_CLIENTS] = {0};


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
    short tunnelindex = 0;


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
            perror("Tunnels: socket");
            continue;
        }


        // avoiding the "Address already in use" error message
        int yes = 1;
        if (setsockopt(control_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == ERROR) {
            perror("setsockopt");
            exit(EXIT_FAILURE);
        }

        if (bind(control_sock, p->ai_addr, p->ai_addrlen) == ERROR) {
            perror("Tunnels: bind");
            close(control_sock);
            continue;
        }

        break; // Bind successful
    }

    if (p == NULL) {
        // Bind successful, print the port number and exit
        fprintf(stderr, "Tunnels: Bind failed, try again with a different port!!\n");
        freeaddrinfo(servinfo);
        return EXIT_FAILURE;
    }


    printf("Tunnels: successfully bound to port %s\n", port);
    freeaddrinfo(servinfo);

    if (listen(control_sock, MAX_CLIENTS) == ERROR) {
        perror("listen");
        return EXIT_FAILURE;
    }

    printf("Tunnels: waiting for connections...\n");

    while (1) {
        // accept connection
        addr_len = sizeof tunnelc;
        if ((control_conn = accept(control_sock, (struct sockaddr *) &tunnelc, &addr_len)) == ERROR) {
            perror("Tunnels: error accepting connection!");
            return EXIT_FAILURE;
        }

        // Fork child process to handle packet forwarding
        pid_t pid = fork();
        if (pid < 0) {
            perror("Error forking process");
            close(control_conn);
            continue;
        } else if (pid == 0) { // Child process code

            short tunnelindex_child = tunnelindex;
            bool success = false;

            // 1st Socket: client<->tunnel
            struct addrinfo *p1;
            int data_sock, attempts = 0;
            struct addrinfo *data_addr;

            char data_port[MAX_PORT_NUM];

            do {
                snprintf(data_port, sizeof(data_port), "%d", atoi("60000") + attempts); // Increment port number

                if ((rv = build_address(ip, data_port, SOCK_DGRAM, &data_addr)) != 0) {
                    fprintf(stderr, "getaddrinfo server: %s\n", gai_strerror(rv));
                    return EXIT_FAILURE;
                }

                // loop through all the results and bind to the first we can
                for (p1 = data_addr; p1 != NULL; p1 = p1->ai_next) {
                    if ((data_sock = socket(p1->ai_family, p1->ai_socktype, p1->ai_protocol)) == ERROR) {
                        perror("Tunnels: socket");
                        continue;
                    }

                    // avoiding the "Address already in use" error message
                    int yes = 1;
                    if (setsockopt(data_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == ERROR) {
                        perror("setsockopt");
                        exit(EXIT_FAILURE);
                    }

                    if (bind(data_sock, p1->ai_addr, p1->ai_addrlen) == ERROR) {
                        perror("Tunnels: bind");
                        close(data_sock);
                        continue;
                    }

                    success = true;
                    break; // Bind successful
                }

                if (p1 == NULL) {
                    // Bind successful, print the port number and exit
                    fprintf(stderr, "Server: maximum number of attempts reached\n");
                    freeaddrinfo(data_addr);
                    return EXIT_FAILURE;
                }

                if (success) {
                    success = false;
                    break;
                }

            } while (++attempts < MAX_ATTEMPTS);

            printf("Tunnels: successfully bound to data port %s | client<->tunnels\n", data_port);
            freeaddrinfo(data_addr);

            // Inform client about the data port
            if (write(control_conn, &data_port, sizeof(data_port)) != sizeof(data_port)) {
                perror("Error writing data port to client");
                close(control_conn);
                close(data_sock);
                exit(EXIT_FAILURE);
            }

            // 2nd Socket: tunnels<->dest
            struct addrinfo *p2;
            attempts = 0;
            int response_sock;
            struct addrinfo *response_addr;

            char response_port[MAX_PORT_NUM];

            do {
                snprintf(response_port, sizeof(response_port), "%d", atoi("64000") + attempts); // Increment port number

                if ((rv = build_address(ip, response_port, SOCK_DGRAM, &response_addr)) != 0) {
                    fprintf(stderr, "getaddrinfo server: %s\n", gai_strerror(rv));
                    return EXIT_FAILURE;
                }

                // loop through all the results and bind to the first we can
                for (p2 = response_addr; p2 != NULL; p2 = p2->ai_next) {
                    if ((response_sock = socket(p2->ai_family, p2->ai_socktype, p2->ai_protocol)) == ERROR) {
                        perror("Tunnels: socket");
                        continue;
                    }


                    // avoiding the "Address already in use" error message
                    int yes = 1;
                    if (setsockopt(response_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == ERROR) {
                        perror("setsockopt");
                        exit(EXIT_FAILURE);
                    }

                    if (bind(response_sock, p2->ai_addr, p2->ai_addrlen) == ERROR) {
                        perror("Tunnels: bind");
                        close(response_sock);
                        continue;
                    }

                    success = true;
                    break; // Bind successful
                }

                if (p2 == NULL) {
                    // Bind successful, print the port number and exit
                    fprintf(stderr, "Tunnels: maximum number of attempts reached\n");
                    freeaddrinfo(response_addr);
                    return EXIT_FAILURE;
                }

                if (success) {
                    success = false;
                    break;
                }


            } while (++attempts < MAX_ATTEMPTS);

            printf("Tunnels: successfully bound to forward port %s for | tunnels<->dest\n", response_port);
            freeaddrinfo(response_addr);

            // adding ports to the struct
            tabentry[tunnelindex_child].srcport = atoi(data_port);
            tabentry[tunnelindex_child].tunnelsport = atoi(response_port);

            // Monitor activity on control_conn, data_sock, and data_conn using select()
            while (1) {
                fd_set readfds;
                FD_ZERO(&readfds);
                FD_SET(control_conn, &readfds);
                FD_SET(data_sock, &readfds);
                FD_SET(response_sock, &readfds);

                // Timeout is NULL to block indefinitely until activity is detected on one of the sockets
                int activity = select(FD_SETSIZE, &readfds, NULL, NULL, NULL);
                if (activity < 0) {
                    perror("Select error");
                    exit(EXIT_FAILURE);
                }

                // Handle activity on control_conn (TCP)
                if (FD_ISSET(control_conn, &readfds)) {
                    // Read termination message from the client
                    if (read(control_conn, buffer, 8) < 0) {
                        perror("Error reading termination message from control connection");
                        exit(EXIT_FAILURE);
                    }

                    // Check if termination message matches the secret key
                    if (strncmp(buffer, secret_key, 8) == 0) {
                        // go through termination process
                        close(control_conn);
                        free_session_entry(tabentry, tunnelindex_child);
                        exit(EXIT_SUCCESS);
                    }
                }

                // Handle activity on data_sock
                if (FD_ISSET(data_sock, &readfds)) {
                    // Receive data from client
                    char data[1024];
                    struct sockaddr_in client_addr;
                    socklen_t client_addr_len = sizeof(client_addr);
                    ssize_t recv_len = recvfrom(data_sock, data, sizeof(data), 0, (struct sockaddr *) &client_addr,
                                                &client_addr_len);
                    if (recv_len < 0) {
                        perror("Tunnels: Error receiving data from client");
                        close(data_sock);
                        close(response_sock);
                        exit(EXIT_FAILURE);
                    }

                    // Forward the received data to the destination
                    struct sockaddr_in dst_addr;
                    memset(&dst_addr, 0, sizeof(dst_addr));
                    dst_addr.sin_family = AF_INET;
                    dst_addr.sin_addr.s_addr = tabentry[tunnelindex].dstaddress;
                    dst_addr.sin_port = tabentry[tunnelindex].dstport;

                    ssize_t send_len = sendto(response_sock, data, recv_len, 0, (struct sockaddr *) &dst_addr,
                                              sizeof(dst_addr));
                    if (send_len < 0) {
                        perror("Tunnels: Error sending data to destination");
                        close(data_sock);
                        close(response_sock);
                        exit(EXIT_FAILURE);
                    }

                }

                // Handle activity on forward_sock
                if (FD_ISSET(response_sock, &readfds)) {
                    // Receive response from destination
                    char data[1024];
                    struct sockaddr_in dst_addr;
                    socklen_t dest_addr_len = sizeof(dst_addr);
                    ssize_t recv_len = recvfrom(response_sock, data, sizeof(data), 0, (struct sockaddr *)&dst_addr, &dest_addr_len);
                    if (recv_len < 0) {
                        perror("Error receiving response from destination");
                        close(data_sock);
                        close(response_sock);
                        exit(EXIT_FAILURE);
                    }

                    // Forward the received response to the client
                    struct sockaddr_in client_addr;
                    memset(&client_addr, 0, sizeof(client_addr));
                    client_addr.sin_family = AF_INET;
                    client_addr.sin_addr.s_addr = tabentry[tunnelindex].srcaddress;
                    client_addr.sin_port = tabentry[tunnelindex].srcport;

                    ssize_t send_len = sendto(data_sock, buffer, recv_len, 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
                    if (send_len < 0) {
                        perror("Error sending response to client");
                        close(data_sock);
                        close(response_sock);
                        exit(EXIT_FAILURE);
                    }
                }
            }

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

            dest_addr = *(unsigned long *) buffer;

            // Read destination port number from the client
            if (read(control_conn, buffer, 2) < 0) {
                perror("Error reading destination port from control connection");
                close(control_conn);
                continue;
            }

            dest_port = *(unsigned short *) buffer;

            // Read source IPv4 address from the client
            if (read(control_conn, buffer, 4) < 0) {
                perror("Error reading source address from control connection");
                close(control_conn);
                continue;
            }

            src_addr = *(unsigned long *) buffer;

            // Find available entry in tabentry[]
            tunnelindex = find_available_entry(tabentry);

            if (tunnelindex != -1) {
                printf("Available entry found at index %d\n", tunnelindex);
            } else {
                printf("Maximum number of forwarding sessions reached\n");
                close(control_conn);
                continue;
            }

            // Update tabentry[] with client request details
            tabentry[tunnelindex].srcaddress = src_addr;
            tabentry[tunnelindex].dstaddress = dest_addr;
            tabentry[tunnelindex].dstport = dest_port;

            // Close control connection in parent process
            close(control_conn);
            printf("New tunneling session established (tunnelindex: %d)\n", tunnelindex);
        }
    }

    // Close control socket
    close(control_sock);

    return 0;
}