/*
** pings.c -- a datagram sockets "server"
*/

#include "message_utils.h"
#include "socket_utils.h"
#include "constants.h"

// arg[0] = command, arg[1] = ip, arg[2] = port
int main(int argc, char *argv[]) {
    if (argc < S_ARG_COUNT) {
        fprintf(stderr, "Usage: %s <IPv6 address> <port number>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *const ip = argv[1];
    char ipv6_address_with_zone[strlen(ip) + 6];
    const char *const initial_port = argv[2];
    char port[MAX_PORT_NUM];

    // Concatenate IPv6 address with the zone identifier
    sprintf(ipv6_address_with_zone, "%s%%eth0", ip);


    int sockfd, rv, numbytes, attempts = 0;
    struct addrinfo *servinfo, *p;
    struct sockaddr_in6 client_addr;
    char s[INET_ADDRSTRLEN];
    uint8_t buf[BUF_MAX_LEN];
    memset(buf, 0, BUF_MAX_LEN);

    socklen_t addr_len;


    do {
        snprintf(port, sizeof(port), "%d", atoi(initial_port) + attempts); // Increment port number

        if ((rv = build_address(ipv6_address_with_zone, port, &servinfo) != 0)) {
            fprintf(stderr, "getaddrinfo server: %s\n", gai_strerror(rv));
            return EXIT_FAILURE;
        }

//         loop through all the results and bind to the first we can
        for (p = servinfo; p != NULL; p = p->ai_next) {
            if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
                perror("Server: socket");
                continue;
            }


            // avoiding the "Address already in use" error message
            int yes = 1;
            if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1) {
                perror("setsockopt");
                exit(EXIT_FAILURE);
            }

            if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
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


    while (1) {
        printf("Server: waiting to recvfrom...\n");

        addr_len = sizeof client_addr;
        if ((numbytes = recvfrom(sockfd, buf, BUF_MAX_LEN, 0, (struct sockaddr *) &client_addr, &addr_len)) == -1) {
            perror("recvfrom");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        if (numbytes == 0)
            continue;


        printf("Server: got a %d bytes packet from %s\n", numbytes, inet_ntop(AF_INET6,
                                                                              &(((struct sockaddr_in6 *) &client_addr)->sin6_addr),
                                                                              s, sizeof s));
        buf[numbytes] = '\0';

        int16_t sq_num;
        uint8_t command;
        decode_packet(buf, &sq_num, &command);

        fflush(stdout);

        // feedback
        switch (command) {
            case 0:
                printf("Sending Immediately\n");
                if (sendto(sockfd, buf, BUF_MAX_LEN, 0, (struct sockaddr *) &client_addr, addr_len) < 0) {
                    perror("sendto");
                    close(sockfd);
                    exit(EXIT_FAILURE);
                }
                break;
            case 1:
                printf("Sending with delay\n");
                usleep(DELAY);
                if (sendto(sockfd, buf, BUF_MAX_LEN, 0, (struct sockaddr *) &client_addr, addr_len) < 0) {
                    perror("sendto with delay");
                    close(sockfd);
                    exit(EXIT_FAILURE);
                }
                break;
            case 2:
                printf("Ignoring Packet\n");
                break;
            default:
                printf("Server: Invalid command from %s:%s, command=%d\n", inet_ntop(AF_INET6,
                                                                                     &(((struct sockaddr_in6 *) &client_addr)->sin6_addr),
                                                                                     s, sizeof s), port, command);
                break;
        }
    }
}