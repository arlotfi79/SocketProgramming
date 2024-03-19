#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>


#include "socket_utils.h"
#include "constants.h"

// arg[0] = command, arg[1] = server ip, arg[2] = server port, arg[3] = client ip
int main(int argc, char *argv[]) {

    if (argc < C_ARG_COUNT) {
        fprintf(stderr, "Usage: %s <IPv6 address of destination> <Destination port number> <IPv6 address of this machine>", argv[0]);
        exit(EXIT_FAILURE);
    }

    int len;
    char input[MAX_ARRAY_LENGTH];
    ssize_t bytes_written;

    // sockets prep
    int sockfd, rv, numbytes;
    struct addrinfo *server_info, *client_info;
    const char *const server_ip = argv[1];
    char server_ipv6_address_with_zone[strlen(server_ip) + 6];
    const char *const server_port = argv[2];
    const char *const client_ip = argv[3];
    char client_ipv6_address_with_zone[strlen(client_ip) + 6];
    const char *const client_port = 0; // to automatically bind it to a free port

    // Concatenate IPv6 address with the zone identifier
    sprintf(server_ipv6_address_with_zone, "%s%%eth0", server_ip);
    sprintf(client_ipv6_address_with_zone, "%s%%eth0", client_ip);

    // Building addresses
    if ((rv = build_address(client_ipv6_address_with_zone, client_port, &client_info) != 0)) {
        fprintf(stderr, "getaddrinfo client: %s\n", gai_strerror(rv));
        return 1;
    }

    if ((rv = build_address(server_ipv6_address_with_zone, server_port, &server_info) != 0)) {
        fprintf(stderr, "getaddrinfo client: %s\n", gai_strerror(rv));
        return 1;
    }

    // Creating socket
    if ((sockfd = create_socket(client_info)) == ERROR)
        return EXIT_FAILURE;

    // Binding socket
    if (bind_socket(client_info, sockfd) == ERROR) {
        if (close(sockfd) == ERROR) perror("close");
        return EXIT_FAILURE;
    }

    // connect to server
    if (connect(sockfd, server_info->ai_addr, server_info->ai_addrlen) == ERROR) {
        if (close(sockfd) == ERROR) perror("close");
        return EXIT_FAILURE;
    }

    // free as the connection is established
    freeaddrinfo(client_info);
    freeaddrinfo(server_info);

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

        // Sending message
        if ((numbytes = send(sockfd, input, len, 0)) == ERROR) {
            perror("client: send");
            exit(EXIT_FAILURE);
        }

        // Setting alarm
        signal(SIGALRM, handle_timeout); // Re-register signal handler
        ualarm(TIMEOUT, 0); // Set microsecond alarm

        // Receiving response from the server
        char server_response[MAX_BUFFER_SIZE];
        if ((numbytes = recv(sockfd, server_response, MAX_BUFFER_SIZE - 1, 0)) == ERROR) {
            perror("client: recv");
            exit(EXIT_FAILURE);
        }

        // Cancel the alarm as response received
        alarm(0);

        // Print the received response from the server
        server_response[numbytes] = '\0';
        printf("Server response: %s\n", server_response);

        exit(EXIT_SUCCESS);
    }
}
