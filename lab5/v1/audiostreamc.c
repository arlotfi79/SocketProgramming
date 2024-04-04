#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <semaphore.h>


#include "../lib/constants.h"
#include "../lib/socket_utils.h"
#include "../lib/congestion_control.h"

sem_t buffer_sem;
struct timeval start_time;
int buffer_occupancy = 0;


int main(int argc, char *argv[]) {
    if (argc != C_ARG_COUNT) {
        fprintf(stderr, "Usage: %s <audiofile> <blocksize> <buffersize> <targetbuf> "
                        "<server-IP> <server-port> <logfileC>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *audiofile = argv[1];
    int blocksize = atoi(argv[2]);
    int buffersize = atoi(argv[3]) * 4096; // Convert to bytes
    int targetbuf = atoi(argv[4]) * 4096; // Convert to bytes
    const char *server_ip = argv[5];
    const char *server_port = argv[6];
    const char *logfileC = argv[7];


    // sockets prep
    int sockfd, rv;
    struct addrinfo *server_info, *client_info;

    // Building addresses
    if ((rv = build_address(NULL, "0", SOCK_DGRAM, &client_info) != 0)) {
        fprintf(stderr, "Client: getaddrinfo client: %s\n", gai_strerror(rv));
        return 1;
    }


    if ((rv = build_address(server_ip, server_port, SOCK_STREAM, &server_info) != 0)) {
        fprintf(stderr, "Client: getaddrinfo server: %s\n", gai_strerror(rv));
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


    // Send audiofile name and blocksize to the server
    if (sendto(sockfd, audiofile, strlen(audiofile), 0, server_info->ai_addr, server_info->ai_addrlen) == -1) {
        perror("Client: Error sending audiofile name");
        return -1;
    }

    if (sendto(sockfd, &blocksize, sizeof(blocksize), 0, server_info->ai_addr, server_info->ai_addrlen) == -1) {
        perror("Client: Error sending blocksize");
        return -1;
    }

    // Buffer for receiving audio data
    sem_init(&buffer_sem, 0, 1);
    struct itimerval timer = {{0, 313000}, {0, 1}};
    setitimer(ITIMER_REAL, &timer, NULL);
    gettimeofday(&start_time, NULL);

    // Loop for receiving audio data
    uint8_t buffer[buffersize];
    while (1) {
        uint8_t block[blocksize];
        int num_bytes_received = recvfrom(sockfd, block, blocksize, 0, NULL, NULL);
        if (num_bytes_received < 0) {
            perror("Client: Error receiving audio data");
            return -1;
        }

        sem_wait(&buffer_sem);
        memcpy(buffer + buffer_occupancy, block, num_bytes_received);
        buffer_occupancy += num_bytes_received;
        sem_post(&buffer_sem);

        // Send feedback packet after each read/write operation
        send_feedback(sockfd, server_info, buffer_occupancy, targetbuf, buffersize);

        // Record buffer occupancy in the logfile
//        fprintf(logfile, "%d\n", buffer_occupancy);
    }

    // Close the logfile and socket
//    fclose(logfile);
    close(sockfd);

}
