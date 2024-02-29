/*
** ssftpd.c -- a datagram sockets "server" for small size file transfer protocol
*/

#include <arpa/inet.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>

#include "socket_utils.h"
#include "ack_handling.h"
#include "constants.h"

// globals
int sockfd;
char *file_content;
unsigned char buf[SERVER_MAX_PAYLOAD_SIZE + 1];
volatile int transmit_mode = 1;

int create_file(char *file_path, int size);

void sigpoll_handler(int signum);

void nack_handler(int signum);

// arg[0] = command, arg[1] = ip, arg[2] = port
int main(int argc, char *argv[]) {

    // creating a sample file for checking
    if (create_file("server_tmp/testfile", FILE_SIZE_BENCH) == -1) {
        printf("Error creating file\n");
        exit(EXIT_FAILURE);
    }


    if (argc < S_ARG_COUNT) {
        fprintf(stderr, "Usage: %s <IPv4 address> <port number>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *const ip = argv[1];
    const char *const initial_port = argv[2];
    char port[MAX_PORT_NUM];


    int rv, numbytes, attempts = 0;
    struct addrinfo *servinfo, *p;
    struct sockaddr_in client_addr;
    char s[INET_ADDRSTRLEN];

    socklen_t addr_len;


    do {
        snprintf(port, sizeof(port), "%d", atoi(initial_port) + attempts); // Increment port number

        if ((rv = build_address(ip, port, &servinfo) != 0)) {
            fprintf(stderr, "getaddrinfo server: %s\n", gai_strerror(rv));
            return EXIT_FAILURE;
        }

        // loop through all the results and bind to the first we can
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

    if (set_non_blocking(sockfd) < 0) {
        perror("server: nonblocking");
        exit(EXIT_FAILURE);
    }

    printf("Server: successfully bound to port %s\n", port);
    freeaddrinfo(servinfo);

    int owner_pid = fcntl(sockfd, F_GETOWN);
    int owner_flags = fcntl(sockfd, F_GETFL);


    // --- alarm for nack ---
    struct sigaction nack;
    memset(&nack, 0, sizeof(nack));
    nack.sa_handler = nack_handler;
    if (sigaction(SIGALRM, &nack, NULL) == -1) {
        perror("Failed to set up SIGALRM handler");
        return EXIT_FAILURE;
    }
    // --- alarm for nack ---

    printf("Server: waiting to recvfrom...\n");
    while (1) {
        struct sigaction sa;
        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = SIG_DFL; // Reset to default action
        sa.sa_flags = 0;
        if (sigaction(SIGIO, &sa, NULL) == -1) {
            perror("Failed to reset SIGIO handler");
            exit(EXIT_FAILURE);
        }

        char filename[MAX_FILENAME_LENGTH + 1]; // +1 for null terminator
        memset(filename, '\0', sizeof(filename));

        addr_len = sizeof client_addr;
        if ((numbytes = recvfrom(sockfd, filename, MAX_FILENAME_LENGTH, 0, (struct sockaddr *) &client_addr,
                                 &addr_len)) == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                // No data available, retry or perform other tasks
                continue;
            } else {
                perror("recvfrom filename");
                close(sockfd);
                exit(EXIT_FAILURE);
            }
        }

        // --- alarm for retransmit ---
        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = sigpoll_handler;
        sa.sa_flags = SA_RESTART | SA_NODEFER; // non-blocking
        if (sigaction(SIGIO, &sa, NULL) == -1) {
            perror("Failed to set up SIGALRM handler");
            return EXIT_FAILURE;
        }
        // --- alarm for retransmit ---

        if (numbytes == 0 || numbytes == 1)
            continue;

        filename[MAX_FILENAME_LENGTH] = '\0'; // Ensure null terminator

        // Remove padding characters ('Z') from the filename
        int original_filename_length = strnlen(filename, MAX_FILENAME_LENGTH + 1);
        while (original_filename_length > 0 && filename[original_filename_length - 1] == 'Z') {
            filename[--original_filename_length] = '\0';
        }

        printf("Server: got a %d bytes packet from %s\n", numbytes, inet_ntop(AF_INET,
                                                                              &(((struct sockaddr_in *) &client_addr)->sin_addr),
                                                                              s, sizeof s));


        // Check if the requested file exists
        char filepath[sizeof(SERVER_TEMP_DIR) + sizeof(filename)];
        strcpy(filepath, SERVER_TEMP_DIR);
        strcat(filepath, filename);
        FILE *file = fopen(filepath, "rb");
        if (file == NULL) {
            fflush(stdout);
            printf("ignoring request\n");
            fprintf(stderr, "File does not exist: %s\n", filename);
            continue;
        }

        // Calculate file size
        fseek(file, 0L, SEEK_END); // move the file position indicator to the end of the file
        unsigned long file_size = ftell(file); // returns the current value of the file position indicator
        rewind(file); // reset the file position indicator to the beginning of the file

        // Read file content into memory
        file_content = (char *) malloc(file_size);
        if (file_content == NULL) {
            perror("malloc");
            fclose(file);
            continue;
        }
        fread(file_content, 1, file_size, file);
        fclose(file);

        // Send file size to client
        unsigned char file_size_packet[SERVER_FIRST_PCK_SIZE];
        memset(file_size_packet, 0, sizeof(file_size_packet));
        file_size_packet[0] = (file_size >> 16) & 0xFF;
        file_size_packet[1] = (file_size >> 8) & 0xFF;
        file_size_packet[2] = file_size & 0xFF;

        if (sendto(sockfd, file_size_packet, SERVER_FIRST_PCK_SIZE, 0, (struct sockaddr *) &client_addr,
                   addr_len) ==
            -1) {
            perror("sendto filesize");
            free(file_content);
            continue;
        }

        printf("Server: file size sent to %s\n", inet_ntop(AF_INET,
                                                           &(((struct sockaddr_in *) &client_addr)->sin_addr),
                                                           s, sizeof s));

        // Send file content to client in chunks
        int sequence_number = 0, bytes_sent = 0;
        transmit_mode = 1;

        while (transmit_mode) {
            while (bytes_sent < file_size) {
                int bytes_to_send = (file_size - bytes_sent > SERVER_MAX_PAYLOAD_SIZE) ? SERVER_MAX_PAYLOAD_SIZE : (
                        file_size - bytes_sent);

                memset(buf, '\0', sizeof buf);

                buf[0] = sequence_number++;
                memcpy(buf + 1, file_content + bytes_sent, bytes_to_send);

                printf("Server: sending packet #%d to %s\n", buf[0], inet_ntop(AF_INET,
                                                                               &(((struct sockaddr_in *) &client_addr)->sin_addr),
                                                                               s, sizeof s));
                if (sendto(sockfd, buf, bytes_to_send + 1, 0, (struct sockaddr *) &client_addr, addr_len) == -1) {
                    perror("sendto packets");
                    continue;
                }

                bytes_sent += bytes_to_send;

                if (bytes_sent >= file_size) {
                    printf("waiting for 2 seconds after sending the last packet\n");
                    alarm(2);
                }

            }
        }


        printf("Server: Finished sending packet to %s\n", inet_ntop(AF_INET,
                                                                    &(((struct sockaddr_in *) &client_addr)->sin_addr),
                                                                    s, sizeof s));

        // Set the process ID to receive SIGIO signals for this socket
        if (fcntl(sockfd, F_SETOWN, owner_pid) == -1) {
            perror("fcntl F_SETOWN");
            exit(EXIT_FAILURE);
        }
        if (fcntl(sockfd, F_SETFL, owner_flags) == -1) {
            perror("fcntl F_SETFL");
            exit(EXIT_FAILURE);
        }

    }

    free(file_content);

}

int create_file(char *file_path, int size) {
    FILE *file = fopen(file_path, "wb");
    if (file == NULL) {
        perror("Error opening file");
        return -1;
    }

    // Write 2560 bytes of data to the file
    for (int i = 0; i < size; i++) {
        fputc('A', file); // Write Anything
    }

    fclose(file);
    printf("File created successfully.\n");
    return 0;
}

// SIGPOLL handler
void sigpoll_handler(int signum) {
    printf("---- ALARM ACTIVATED ----\n");

    int numbytes = 0;
    struct sockaddr_in client_addr;
    unsigned char nack_packet[1];

    socklen_t addr_len;
    addr_len = sizeof client_addr;
    if ((numbytes = recvfrom(sockfd, nack_packet, 1, 0, (struct sockaddr *) &client_addr,
                             &addr_len)) == -1) {
        perror("recvfrom async");
        close(sockfd);
        exit(EXIT_FAILURE);
    }


    printf("Received retransmit request! Resending packet #{%d}...\n", nack_packet[0]);

    memset(buf, '\0', sizeof buf);
    buf[0] = nack_packet[0];
    memcpy(buf + 1, file_content + SERVER_MAX_PAYLOAD_SIZE * nack_packet[0], SERVER_MAX_PAYLOAD_SIZE);

    if (sendto(sockfd, buf, SERVER_MAX_PAYLOAD_SIZE + 1, 0, (struct sockaddr *) &client_addr, addr_len) == -1) {
        perror("sendto async");
        free(file_content);
    }

    printf("---- ALARM RESET ----\n");
    printf("waiting for 2 seconds after responding to the last nack request\n");
    alarm(2);
}

void nack_handler(int signum) {
    if (signum == SIGALRM) {
    transmit_mode = 0;
    printf("changed transmit_mode to 0\n");
    }
}