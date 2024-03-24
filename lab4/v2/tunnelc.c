// tunnelc.c - Single-hop tunneling client

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>


#include "constants.h"
#include "socket_utils.h"


int main(int argc, char *argv[]) {
    if (argc != C_ARG_COUNT) {
        fprintf(stderr, "Usage: %s <tunneling server IPv4 address> <tunneling server port number> "
                        "<secret key> <client IPv4 address> <final destination IPv4 address> <final destination port number>\n",
                argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *const tunneling_ip = argv[1];
    const char *const tunneling_port = argv[2];

    const char *const secret_key = argv[3];

    unsigned long client_ip = inet_addr(argv[4]);

    unsigned long destination_ip = inet_addr(argv[5]);
    unsigned short destination_port = htons(atoi(argv[6]));


    // sockets prep
    int sockfd, rv;
    struct addrinfo *server_info, *client_info;

    // Building addresses
    if ((rv = build_address("127.0.0.1", "0", SOCK_STREAM, &client_info) != 0)) {
        fprintf(stderr, "Tunnelc: getaddrinfo client: %s\n", gai_strerror(rv));
        return 1;
    }


    if ((rv = build_address(tunneling_ip, tunneling_port, SOCK_STREAM, &server_info) != 0)) {
        fprintf(stderr, "Tunnelc: getaddrinfo server: %s\n", gai_strerror(rv));
        return 1;
    }

    // Creating socket
    if ((sockfd = create_socket(client_info)) == -1)
        return EXIT_FAILURE;

    // Binding socket
    if (bind_socket(client_info, sockfd) == -1) {
        if (close(sockfd) == -1) perror("close");
        return -1;
    }


    // Connect to server
    if (connect(sockfd, server_info->ai_addr, server_info->ai_addrlen) == ERROR) {
        if (close(sockfd) == ERROR) perror("close");
        return EXIT_FAILURE;
    }

    // Send request to establish tunneling session
    char request_type = 'c';
    write(sockfd, &request_type, sizeof(request_type));

    // Send secret key
    write(sockfd, secret_key, strlen(secret_key));

    // Send final destination's IPv4 address (4B), port number (2B), and client IPv4 address (4B)
    write(sockfd, &destination_ip, sizeof(destination_ip));
    write(sockfd, &destination_port, sizeof(destination_port));
    write(sockfd, &client_ip, sizeof(client_ip));

    // Receive port number from the tunneling server
    char port_message[MAX_PORT_NUM];
    read(sockfd, port_message, sizeof(port_message));

    // Print data port number
    printf("Data port number: %s\n", port_message);

    // Close TCP socket
    close(sockfd);

    return 0;
}
