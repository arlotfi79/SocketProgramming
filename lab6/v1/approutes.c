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
        fprintf(stderr, "Usage: %s <IPv4 address> <port number>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *const ip = argv[1];
    const char *const port = argv[2];
    short tunnelindex = 0;


    int control_sock, control_conn, rv;
    struct addrinfo *servinfo, *p;
    struct sockaddr_in tunnelc;
    socklen_t addr_len;

    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));


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


        // Read destination IPv4 address from the client
        unsigned long next_hop_address;
        if (read(control_conn, &next_hop_address, sizeof(next_hop_address)) <= 0) {
            perror("Error reading destination address from control connection\n");
            close(control_conn);
            continue;
        }

        // Read destination port number from the client
        unsigned short next_hop_port;
        if (read(control_conn, &next_hop_port, sizeof(next_hop_port)) <= 0) {
            perror("Error reading destination port from control connection\n");
            close(control_conn);
            continue;
        }

        // Find available entry in tabentry[]
        tunnelindex = find_available_entry(tabentry);

        if (tunnelindex != -1) {
            printf("Available entry found at index %d\n", tunnelindex);
        } else {
            printf("Maximum number of forwarding sessions reached\n");
            close(control_conn);
            continue;
        }

        // Store the values in data structure
        tabentry[tunnelindex].next_hop_address = next_hop_address;
        tabentry[tunnelindex].next_hop_port = ntohs(next_hop_port);

        // Fork child process to handle packet forwarding
        pid_t pid = fork();
        if (pid < 0) {
            perror("Error forking process");
            close(control_conn);
            continue;
        } else if (pid == 0) { // Child process code

            short tunnelindex_child = tunnelindex;
            bool success = false;

            // 1st Socket: last_server<->tunnel
            struct addrinfo *p1;
            int data_sock, attempts = 1;
            struct addrinfo *srv_addr;

            char port_in[MAX_PORT_NUM];

            do {
                snprintf(port_in, sizeof(port_in), "%d", atoi(port) + attempts); // Increment port number


                if ((rv = build_address(ip, port_in, SOCK_DGRAM, &srv_addr)) != 0) {
                    fprintf(stderr, "getaddrinfo server: %s\n", gai_strerror(rv));
                    return EXIT_FAILURE;
                }

                // loop through all the results and bind to the first we can
                for (p1 = srv_addr; p1 != NULL; p1 = p1->ai_next) {
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
                    freeaddrinfo(srv_addr);
                    return EXIT_FAILURE;
                }

                if (success) {
                    success = false;
                    break;
                }

            } while (++attempts < MAX_ATTEMPTS);

            printf("Tunnels: successfully bound to port_in %s | client<->tunnels\n", port_in);
            freeaddrinfo(srv_addr);

            // Inform client about the data port
            if (write(control_conn, &port_in, sizeof(port_in)) != sizeof(port_in)) {
                perror("Error writing data port to client");
                close(control_conn);
                close(data_sock);
                exit(EXIT_FAILURE);
            }

            // 2nd Socket: tunnels<->dest
            struct addrinfo *p2;
            attempts = 2;
            int response_sock;
            struct addrinfo *dest_addr;

            char port_out[MAX_PORT_NUM];

            do {
                snprintf(port_out, sizeof(port_out), "%d", atoi(port) + attempts); // Increment port number

                if ((rv = build_address(ip, port_out, SOCK_DGRAM, &dest_addr)) != 0) {
                    fprintf(stderr, "getaddrinfo server: %s\n", gai_strerror(rv));
                    return EXIT_FAILURE;
                }

                // loop through all the results and bind to the first we can
                for (p2 = dest_addr; p2 != NULL; p2 = p2->ai_next) {
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
                    freeaddrinfo(dest_addr);
                    return EXIT_FAILURE;
                }

                if (success) {
                    success = false;
                    break;
                }


            } while (++attempts < MAX_ATTEMPTS);

            printf("Tunnels: successfully bound to port_out %s for | tunnels<->dest\n", port_out);
            freeaddrinfo(dest_addr);


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
                    int numbbytes = 0;
                    // Read termination message from the client
                    if ((numbbytes = read(control_conn, buffer, 8)) < 0) {
                        perror("Error reading termination message from control connection");
                        exit(EXIT_FAILURE);
                    }

                    if (numbbytes > 0) {
                        // go through termination process
                        close(control_conn);
                        free_session_entry(tabentry, tunnelindex_child);
                        exit(EXIT_SUCCESS);
                    }
                }

                // Handle activity on data_sock
                if (FD_ISSET(data_sock, &readfds)) {
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

                    printf("Received data from client with ip: %s and port: %d\n", inet_ntoa(client_addr.sin_addr),
                           ntohs(client_addr.sin_port));

                    tabentry[tunnelindex_child].last_hop_address = inet_addr(inet_ntoa(client_addr.sin_addr));
                    tabentry[tunnelindex_child].last_hop_port = ntohs(client_addr.sin_port);

                    // converting
                    struct in_addr addr;
                    addr.s_addr = tabentry[tunnelindex_child].next_hop_address;
                    char dst_ip_str[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &addr, dst_ip_str, INET_ADDRSTRLEN);

                    char dstport_str[MAX_PORT_NUM]; // Assuming a maximum port length of 5 digits
                    sprintf(dstport_str, "%hu", tabentry[tunnelindex_child].next_hop_port);

                    // Forward the received data to the destination
                    struct addrinfo *next_hop_addr;
                    if ((rv = build_address(dst_ip_str, dstport_str, SOCK_DGRAM, &next_hop_addr) != 0)) {
                        fprintf(stderr, "getaddrinfo server: %s\n", gai_strerror(rv));
                        return EXIT_FAILURE;
                    }

                    ssize_t send_len = sendto(response_sock, data, recv_len, 0, next_hop_addr->ai_addr, next_hop_addr->ai_addrlen);

                    if (send_len < 0) {
                        perror("Tunnels: Error sending data to destination");
                        close(data_sock);
                        close(response_sock);
                        exit(EXIT_FAILURE);
                    }

                    printf("Forwarded data to destination with ip: %s and port: %s\n", dst_ip_str, dstport_str);

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

                    printf("Received response from destination with ip: %s and port: %d\n", inet_ntoa(dst_addr.sin_addr),
                           ntohs(dst_addr.sin_port));

                    // converting
                    struct in_addr addr;
                    addr.s_addr = tabentry[tunnelindex_child].last_hop_address;
                    char src_ip_str[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &addr, src_ip_str, INET_ADDRSTRLEN);

                    char srcport_str[MAX_PORT_NUM]; // Assuming a maximum port length of 5 digits
                    sprintf(srcport_str, "%hu", tabentry[tunnelindex_child].last_hop_port);

                    // Forward the received data to the destination
                    struct addrinfo *last_hop_addr;
                    if ((rv = build_address(src_ip_str, srcport_str, SOCK_DGRAM, &last_hop_addr) != 0)) {
                        fprintf(stderr, "getaddrinfo server: %s\n", gai_strerror(rv));
                        return EXIT_FAILURE;
                    }

                    ssize_t send_len = sendto(data_sock, data, recv_len, 0, last_hop_addr->ai_addr, last_hop_addr->ai_addrlen);

                    if (send_len < 0) {
                        perror("Error sending response to client");
                        close(data_sock);
                        close(response_sock);
                        exit(EXIT_FAILURE);
                    }

                    printf("Forwarded response to client with ip: %s and port: %s\n", src_ip_str, srcport_str);
                }
            }

            // Terminate child process
            exit(EXIT_SUCCESS);
        } else { // Parent process code
            close(control_conn);
        }
    }

    // Close control socket
    close(control_sock);

    return 0;
}