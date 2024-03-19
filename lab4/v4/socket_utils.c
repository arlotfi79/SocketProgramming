#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include "socket_utils.h"

// generating error messages
void error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

void handle_timeout(int signum) {
    printf("Timeout: No response from server. Signal number: %d\n", signum);
    exit(1);
}

int build_address(const char *const ip, const char *const port, struct addrinfo **info) {
    int rv;
    struct addrinfo hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET6; // AF_INET = IPV4, AF_INET6 = IPV6, AF_UNSPEC = IPV4/IPV6
    hints.ai_flags = AI_PASSIVE; // Bind to all available interfaces
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(ip, port, &hints, info)) != 0)
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));

    return rv;
}


int create_socket(const struct addrinfo *const info) {
    int sockfd = -1;
    const struct addrinfo *p;

    for (p = info; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("socket error");
            continue;
        }

        // avoiding the "Address already in use" error message
        int yes = 1;
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1) {
            perror("setsockopt");
            exit(EXIT_FAILURE);
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "failed to create socket\n");
        return -1;
    }

    return sockfd;
}

int bind_socket(const struct addrinfo *const info, const int sockfd) {
    const struct addrinfo *p;

    for (p = info; p != NULL; p = p->ai_next) {

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            perror("bind error");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "failed to bind socket\n");
        return -1;
    }
    return 0;
}

