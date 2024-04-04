#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdbool.h>
#include <time.h>


#include "../lib/socket_utils.h"
#include "../lib/constants.h"
#include "../lib/congestion_control.h"

int main(int argc, char *argv[]) {

    if (argc < S_ARG_COUNT) {
        fprintf(stderr, "Usage: %s <lambda> <epsilon> <gamma> <beta> <logfileS> <server IPv4 address> <server-port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    double lambda = atoi(argv[1]);
    const double epsilon = atoi(argv[2]);
    const double gamma = atoi(argv[3]);
    const double beta = atoi(argv[4]);
    const char *const logfileS = argv[5];
    const char *const ip = argv[6];
    const char *const initial_port = argv[7];
    char port[MAX_PORT_NUM];

    // Calculate initial packet_interval
    int packet_interval = (int)(1000 / lambda);

    int rv, sockfd, attempts = 0;
    struct addrinfo *servinfo, *p;
    struct sockaddr_in client_addr;

    bool success = false;
    do {
        snprintf(port, sizeof(port), "%d", atoi(initial_port) + attempts); // Increment port number

        if ((rv = build_address(ip, port, SOCK_DGRAM, &servinfo) != 0)) {
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

            success = true;
            break; // Bind successful
        }

        if (p == NULL) {
            // Bind successful, print the port number and exit
            fprintf(stderr, "Server: maximum number of attempts reached\n");
            freeaddrinfo(servinfo);
            return EXIT_FAILURE;
        }

        if (success) {
            success = false;
            break;
        }


    } while (++attempts < MAX_ATTEMPTS);

    printf("Server: successfully bound to port %s\n", port);
    freeaddrinfo(servinfo);

    // Main loop
    unsigned short buffer_state;

    while (1) {
        unsigned int len = sizeof(client_addr);
        char filename[FILENAME_LENGTH + 1];
        int blocksize;

        // Receive client request
        int num_bytes_received = recvfrom(sockfd, filename, FILENAME_LENGTH, 0, (struct sockaddr *) &client_addr, &len);
        if (num_bytes_received < 0) {
            perror("Server: Error in receiving");
            continue;
        }
        printf("Server: received request for file %s\n", filename);

        // Null-terminate the filename
        filename[num_bytes_received] = '\0';

        // Receive blocksize
        if (recvfrom(sockfd, &blocksize, sizeof(blocksize), 0, (struct sockaddr *) &client_addr, &len) < 0) {
            perror("Server: Error in receiving blocksize");
            continue;
        }
        printf("Server: received blocksize %d\n", blocksize);

        // Check filename validity
        if (strlen(filename) != FILENAME_LENGTH || strchr(filename, ' ') != NULL) {
            printf("Server: Invalid filename: %s\n", filename);
            continue;
        }
        printf("Server: filename is valid\n");

        // Fork a child process to handle streaming
        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            close(sockfd);
            int child_sockfd;
            struct addrinfo *child_servinfo;

            // Create a new UDP socket for streaming

            // Building addresses
            if ((rv = build_address(ip, "0", SOCK_DGRAM, &child_servinfo) != 0)) {
                fprintf(stderr, "Client: getaddrinfo client: %s\n", gai_strerror(rv));
                return 1;
            }

            // Creating socket
            if ((child_sockfd = create_socket(child_servinfo)) == -1)
                return EXIT_FAILURE;

            // Binding socket
            if (bind_socket(child_servinfo, child_sockfd) == -1) {
                if (close(child_sockfd) == -1) perror("close");
                return -1;
            }

            printf("Server: child process successfully bound to port %s\n", port);
            freeaddrinfo(child_servinfo);

            // ----- Handle streaming -----
            // Open file
            FILE *file = fopen(filename, "rb");
            if (file == NULL) {
                perror("Server: Error opening file");
                exit(1);
            }

            uint8_t buffer[blocksize];
            size_t bytes_read;

            int packet_counter = 0;
            while ((bytes_read = fread(buffer, 1, blocksize, file)) > 0) {
                if (sendto(child_sockfd, buffer, bytes_read, 0, (struct sockaddr *) &client_addr, len) < 0) {
                    perror("Server: Error sending file");
                    close(child_sockfd);
                    fclose(file);
                    return -1;
                }
                printf("Server: sent packet #%d\n", packet_counter);

                // Receive feedback from client
                if (recvfrom(child_sockfd, &buffer_state, sizeof(buffer_state), 0, (struct sockaddr *) &client_addr, &len) < 0) {
                    perror("Server: Error receiving feedback");
                    close(child_sockfd);
                    fclose(file);
                    return -1;
                }

                // Adjust sending rate based on feedback
                lambda = adjust_sending_rate(lambda, epsilon, gamma, beta, buffer_state);
                printf("Server: adjusted lambda to %f\n", lambda);

                // Calculate new packet interval
                packet_interval = (int)(1000.0 / lambda);

                // Sleep for packet_interval milliseconds before next packet transmission
                struct timespec ts;
                ts.tv_sec = packet_interval / 1000;
                ts.tv_nsec = (packet_interval % 1000) * 1000000;

                if (nanosleep(&ts, NULL) < 0) {
                    perror("Server: Error sleeping");
                    close(child_sockfd);
                    fclose(file);
                    return -1;
                }
                packet_counter++;
            }
            // ----- Handle streaming -----

            close(child_sockfd);
            exit(0);
        } else if (pid < 0) {
            perror("Error forking");
            continue;
        }

        // Parent process continues to listen for new client requests
    }


    // Close socket
    close(sockfd);
    return 0;

}