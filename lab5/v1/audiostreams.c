#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>


#include "../lib/socket_utils.h"
#include "../lib/constants.h"
#include "../lib/congestion_control.h"


// Run -> ./audiostreams.bin 1000 0.1 0.1 0.1  logFileS 127.0.0.1 5000
int main(int argc, char *argv[]) {

    if (argc < S_ARG_COUNT) {
        fprintf(stderr, "Usage: %s <lambda> <epsilon> <gamma> <beta> <logfileS> <server IPv4 address> <server-port>\n",
                argv[0]);
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
    int packet_interval = (int) (1000 / lambda);

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

    // Array to store lambda values
    double lambda_values[MAX_PACKETS];
    double packet_intervals[MAX_PACKETS];
    double elapsed_times[MAX_PACKETS];
    struct timeval start_time, current_time;
    gettimeofday(&start_time, NULL); // Get start time

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
        if (strlen(filename) > FILENAME_LENGTH || strchr(filename, ' ') != NULL) {
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

            // Get the socket address
            struct sockaddr_in sin;
            socklen_t sin_len = sizeof(sin);
            if (getsockname(child_sockfd, (struct sockaddr *) &sin, &sin_len) == -1) {
                perror("getsockname");
                return -1;
            }

            // Print the port number
            printf("Server: child process successfully bound to port %d\n", ntohs(sin.sin_port));

            freeaddrinfo(child_servinfo);

            // ----- Handle streaming -----
            // Open file
            // Append the relative path to the filename
            char relative_path[100]; // Assuming the relative path is within 50 characters
            sprintf(relative_path, "../%s", filename);
            FILE *file = fopen(relative_path, "rb");
            if (file == NULL) {
                perror("Server: Error opening file");
                exit(1);
            }

            uint8_t buffer[blocksize];
            size_t bytes_read;

            int packet_counter = 0;
            int expected_packets = calculate_expected_number_of_packets(relative_path, blocksize);
            printf("Server: Expected number of packets: %d\n", expected_packets);
            while ((bytes_read = fread(buffer, 1, blocksize, file)) > 0 && packet_counter < expected_packets) {
                if (sendto(child_sockfd, buffer, bytes_read, 0, (struct sockaddr *) &client_addr, len) < 0) {
                    perror("Server: Error sending file");
                    close(child_sockfd);
                    fclose(file);
                    return -1;
                }
                printf("Server: sent packet #%d\n", packet_counter);
                memset(buffer, 0, blocksize); // Clear buffer

                // Receive feedback from client
                printf("Server: waiting for feedback\n");
                if (recvfrom(child_sockfd, &buffer_state, sizeof(buffer_state), 0, (struct sockaddr *) &client_addr,
                             &len) < 0) {
                    perror("Server: Error receiving feedback");
                    close(child_sockfd);
                    fclose(file);
                    return -1;
                }
                printf("Server: received feedback! buffer state value: %d\n", buffer_state);

                // Adjust sending rate based on feedback
                lambda = adjust_sending_rate(lambda, epsilon, gamma, beta, buffer_state);
                printf("Server: adjusted lambda to %f\n", lambda);


                // Calculate new packet interval
                packet_interval = (int) (1000.0 / lambda);

                // --- Logging ---
                // Get current time
                gettimeofday(&current_time, NULL);
                // Calculate elapsed time in milliseconds
                double elapsed_time = ((current_time.tv_sec - start_time.tv_sec) * 1000.0) +
                                      ((current_time.tv_usec - start_time.tv_usec) / 1000.0);
                // Store lambda value
                elapsed_times[packet_counter] = elapsed_time;
                lambda_values[packet_counter] = lambda;
                packet_intervals[packet_counter] = packet_interval;
                // --- Logging ---


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
                printf("-----------------------------\n");
            }
            // Send 5 empty packets to signify end of session
            printf("Server: Sending end of session packets (5 empty packets)\n");
            for (int i = 0; i < 5; i++) {
                if (sendto(child_sockfd, NULL, 0, 0, (struct sockaddr *) &client_addr, len) < 0) {
                    perror("Server: Error sending end of session packet");
                    close(child_sockfd);
                    fclose(file);
                    return -1;
                }
            }

            // ----- Handle streaming -----

            // Log lambda values
            char logfile_fullname[100]; // Assuming the relative path is within 50 characters
            sprintf(logfile_fullname, "%s.csv", logfileS);
            FILE *log_file = fopen(logfile_fullname, "w");
            if (log_file == NULL) {
                perror("Error opening log file");
                exit(EXIT_FAILURE);
            }
            printf("Server: Logging lambda values to %s\n", logfileS);

            double normalized_time = 0.0;
            fprintf(log_file, "%s, %s, %s\n", "time", "lambda", "packet_interval");
            for (int i = 0; i < packet_counter; ++i) {
                normalized_time = ((i == 0) ? 0.0 : elapsed_times[i]);
                fprintf(log_file, "%.3f, %.3f, %.3f\n", normalized_time, lambda_values[i], packet_intervals[i]);
            }
            printf("Server: Logging complete\n");

            fclose(log_file);
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