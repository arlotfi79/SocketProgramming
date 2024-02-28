/*
** ssftp.c -- a datagram "client" for small size file transfer protocol
*/

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <sys/time.h>

#include "socket_utils.h"
#include "ack_handling.h"
#include "constants.h"


// arg[0] = command, arg[1] = filename, arg[2] = server ip, arg[2] = server port, arg[4] = client ip, arg[5] = payload_size
int main(int argc, char *argv[]) {

    if (argc < S_ARG_COUNT) {
        fprintf(stderr, "Usage: %s <filename> <IPv4 address of destination> "
                        "<Destination port number> <IPv4 address of this machine>"
                        "<Payload size>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    //prepare filename
    char filename[MAX_FILENAME_LENGTH + 1]; // +1 for null terminator
    strncpy(filename, argv[1], MAX_FILENAME_LENGTH); // Copy at most MAX_FILENAME_LENGTH characters
    filename[MAX_FILENAME_LENGTH] = '\0'; // Ensure null terminator

    int filename_length = strlen(filename);
    for (int i = filename_length; i < MAX_FILENAME_LENGTH; i++) { // Pad filename if shorter than 10 characters
        filename[i] = 'Z';
    }

    // sockets prep
    int sockfd, rv, numbytes;
    struct addrinfo *server_info, *client_info;
    const char *const server_ip = argv[2];
    const char *const server_port = argv[3];
    const char *const client_ip = argv[4];
    const int segment_size = atoi(argv[5]);
    const char *const client_port = 0; // to automatically bind it to a free port

    // Building addresses
    if ((rv = build_address(client_ip, client_port, &client_info) != 0)) {
        fprintf(stderr, "getaddrinfo client: %s\n", gai_strerror(rv));
        return 1;
    }

    if ((rv = build_address(server_ip, server_port, &server_info) != 0)) {
        fprintf(stderr, "getaddrinfo client: %s\n", gai_strerror(rv));
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


    // --- alarm for not receiving from server ---
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_timeout;
    if (sigaction(SIGALRM, &sa, NULL) == -1) {
        perror("Failed to set up SIGALRM handler");
        return EXIT_FAILURE;
    }
    ualarm(TIMEOUT, 0); // Set microsecond alarm
    // --- alarm for not receiving from server ---

    // Start time measurement
    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);

    // Sending message
    if ((numbytes = sendto(sockfd, filename, MAX_FILENAME_LENGTH, 0, server_info->ai_addr, server_info->ai_addrlen)) ==
        -1) {
        perror("client: sendto");
        exit(EXIT_FAILURE);
    }

    printf("client: sent %d bytes to %s:%s\n", numbytes, server_ip, server_port);

    // Getting file size from server
    unsigned char file_size_packet[SERVER_FIRST_PCK_SIZE];
    memset(file_size_packet, 0, sizeof(file_size_packet));

    do {
        numbytes = recvfrom(sockfd, file_size_packet, SERVER_MAX_PAYLOAD_SIZE, 0, NULL, NULL);
    } while (numbytes == -1 && errno == EINTR);

    if (numbytes != SERVER_FIRST_PCK_SIZE) {
        fprintf(stderr, "Invalid number of bytes received.\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Cancel the alarm as response received
    alarm(0);

    // --- alarm for retransmit ---
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = alarm_handler;
    if (sigaction(SIGALRM, &sa, NULL) == -1) {
        perror("Failed to set up SIGALRM handler");
        return EXIT_FAILURE;
    }
    ualarm(RETRANSMIT_TIMEOUT, RETRANSMIT_TIMEOUT); // Set microsecond alarm
    // --- alarm for retransmit ---


    // creating the file_size from the packet received
    unsigned long file_size = (file_size_packet[0] << 16) | (file_size_packet[1] << 8) | file_size_packet[2];

    if (file_size > MAX_FILE_SIZE) {
        fprintf(stderr, "Invalid file size received.\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Getting the file content from server
    int payload_size = atoi(argv[5]), total_bytes_received = 0;

    char *file_content = (char *) malloc(file_size);
    if (file_content == NULL) {
        perror("malloc");
        close(sockfd);
        exit(1);
    }
    memset(file_content, 0, file_size);

    unsigned char buf[payload_size + 1];
    memset(buf, 0, sizeof(buf));


    // NACK initialization
    PacketTracker client_tracker;
    init_packet_tracker(&client_tracker, NOT_RECEIVED);
    unsigned short seq_min = 0;
    unsigned short seq_max = 0;

    // Receive from server
    while (1) {
        if ((numbytes = recvfrom(sockfd, buf, payload_size + 1, 0, NULL, NULL)) ==
            -1) {
            if (errno == EINTR)  // recvfrom was interrupted by a signal, handle it and continue the loop
                continue;
            else { // An error other than EINTR occurred, handle it
                perror("client: recvfrom");
                exit(EXIT_FAILURE);
            }
        }

        // Extract sequence number from the first byte of the buffer
        int sequence_number = buf[0];

        if (sequence_number == seq_min && seq_min == seq_max) {
            seq_max = ++seq_min;
        } else if (sequence_number == seq_min && seq_min != seq_max) {
            seq_min++;
        } else if (sequence_number >= seq_max) {
            seq_max = sequence_number + 1;
        }


        mark_packet(&client_tracker, sequence_number, RECEIVED);


        // Copy file content (excluding sequence number) to file_content array
        memcpy(file_content + sequence_number * payload_size, buf + 1,
               numbytes - 1); // handles the pck if not even inorder
        total_bytes_received += numbytes - 1;

        // 0 - 25
        // Check for missing blocks and request retransmission
        if (alarm_flag) {
            printf("---- ALARM ACTIVATED ----\n");
            // Check for gaps in the received packets
            if (seq_min < seq_max) {
                printf("seqmin: %d, seqmax: %d\n", seq_min, seq_max);
                // Determine which packets are missing within the gap
                for (unsigned short i = seq_min; i < seq_max; i++) {
                    if (packet_status(&client_tracker, i) == NOT_RECEIVED) {
                        // Send NACK to request retransmission
                        unsigned char nack_packet[1];
                        nack_packet[0] = i;
                        if (sendto(sockfd, nack_packet, sizeof(nack_packet), 0, server_info->ai_addr,
                                   server_info->ai_addrlen) == -1) {
                            perror("client: sendto\n");
                            exit(EXIT_FAILURE);
                        }
                        printf("packet#{%d} is lost! Retransmission request sent to %s\n", i, server_ip);
                    }
                }
            }

            ualarm(RETRANSMIT_TIMEOUT, RETRANSMIT_TIMEOUT); // Set alarm for next check

            printf("---- ALARM RESET ----\n");

        }

        if (seq_min < seq_max) {
            unsigned short next_expected_seq = seq_min;

            while (next_expected_seq < seq_max &&
            packet_status(&client_tracker, next_expected_seq) == RECEIVED)
                seq_min = ++next_expected_seq;
        }

        if (seq_min >= file_size / segment_size)
            break;

    }

    // End time measurement
    gettimeofday(&end_time, NULL);

    // Save file content to disk
    // Remove padding characters ('Z') from the filename
    int original_filename_length = strnlen(filename, MAX_FILENAME_LENGTH + 1);
    while (original_filename_length > 0 && filename[original_filename_length - 1] == 'Z') {
        filename[--original_filename_length] = '\0';
    }

    char filepath[sizeof(CLIENT_TEMP_DIR) + sizeof(filename)];
    strcpy(filepath, CLIENT_TEMP_DIR);
    strcat(filepath, filename);

    FILE *file = fopen(filepath, "wb");
    if (file == NULL) {
        perror("fopen");
        close(sockfd);
        free(file_content);
        exit(1);
    }
    fwrite(file_content, 1, file_size, file);
    fclose(file);

    printf("File transfer complete.\n");

    // Free
    free(file_content);
    freeaddrinfo(server_info);
    freeaddrinfo(client_info);
    close(sockfd);


    // Calculate completion time in milliseconds
    double completion_time_ms =
            (end_time.tv_sec - start_time.tv_sec) * 1000 + (end_time.tv_usec - start_time.tv_usec) / 1000;

    // Calculate transfer speed in bits per second
    double transfer_speed_bps = (double) (file_size * 8) / ((double) completion_time_ms / 1000);

    // Calculate percentage of bytes not received

    double percentage_missing = check_transfer_success_rate(&client_tracker, seq_max);
    printf("-------------- RESULTS --------------\n");
    printf("Completion time: %f milliseconds\n", completion_time_ms);
    printf("Transfer speed: %.2f bps\n", transfer_speed_bps);
    printf("Percentage of bytes not received: %.2f%%\n", percentage_missing);

    return 0;
}
