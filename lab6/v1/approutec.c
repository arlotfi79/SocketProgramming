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
#include "utils.h"


int main(int argc, char *argv[]) {
    if (argc != C_ARG_COUNT) {
        fprintf(stderr, "Usage: %s\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int len;
    char input[MAX_ARRAY_LENGTH];
    char *args[5];
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
        return EXIT_FAILURE;
    }

    input[len - 1] = '\0';            // case: command entered
    parse_command(input, args, " ");

    char *router_ip = args[0];
    char *tcp_port = args[1];
    unsigned long next_hop_IP = inet_addr(args[2]);
    unsigned short next_hop_port = htons(atoi(args[3]));


    // sockets prep
    int sockfd, rv;
    struct addrinfo *server_info, *client_info;

    // Building addresses
    if ((rv = build_address(NULL, "0", SOCK_STREAM, &client_info) != 0)) {
        fprintf(stderr, "Tunnelc: getaddrinfo client: %s\n", gai_strerror(rv));
        return 1;
    }


    if ((rv = build_address(router_ip, tcp_port, SOCK_STREAM, &server_info) != 0)) {
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


    // Send final destination's IPv4 address (4B), port number (2B), and client IPv4 address (4B)
    if (write(sockfd, &next_hop_IP, sizeof(next_hop_IP)) < 0) {
        printf("Tunnelc: Error sending next_hop_IP. %lu\n", next_hop_IP);
        close(sockfd);
    }

    if (write(sockfd, &next_hop_port, sizeof(next_hop_port)) < 0) {
        printf("Tunnelc: Error sending next_hop_port. %hu\n", next_hop_port);
        close(sockfd);
    }

    // Receive port number from the tunneling server
    char port_message[MAX_PORT_NUM];
    if (read(sockfd, port_message, sizeof(port_message)) < 0) {
        printf("Tunnelc: Error reading assigned port. %s\n", port_message);
        close(sockfd);
    }

    // Print data port number
    printf("Server port number: %s\n", port_message);

    // Close TCP socket
    close(sockfd);

    return 0;
}
