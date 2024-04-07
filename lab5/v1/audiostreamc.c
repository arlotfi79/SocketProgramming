#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <semaphore.h>
#include <errno.h>


#include "../lib/constants.h"
#include "../lib/socket_utils.h"
#include "../lib/congestion_control.h"
#include "../lib/concurrency_utils.h"
#include "../lib/queue.h"

sem_t *buffer_sem;
struct timeval start_time;

// TODO: implement reading audio data from the buffer and writing it to the audio device
int main(int argc, char *argv[]) {
    if (argc != C_ARG_COUNT) {
        fprintf(stderr, "Usage: %s <audiofile> <blocksize> <buffersize> <targetbuf> "
                        "<server-IP> <server-port> <logfileC>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *audiofile = argv[1];
    int blocksize = atoi(argv[2]);
    int buffersize = atoi(argv[3]);
    int targetbuf = atoi(argv[4]);
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


    if ((rv = build_address(server_ip, server_port, SOCK_DGRAM, &server_info) != 0)) {
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
    printf("Client: Sent audiofile name\n");

    if (sendto(sockfd, &blocksize, sizeof(blocksize), 0, server_info->ai_addr, server_info->ai_addrlen) == -1) {
        perror("Client: Error sending blocksize");
        return -1;
    }
    printf("Client: Sent blocksize\n");

    // semaphore for receiving audio data
    buffer_sem = create_semaphore("/buffer_sem");

//    struct itimerval timer = {{0, 313000}, {0, 1}};
//    setitimer(ITIMER_REAL, &timer, NULL);
//    gettimeofday(&start_time, NULL);

    // Loop for receiving audio data
    Queue* buffer = createQueue(buffersize);
    uint8_t block[blocksize];
    socklen_t len = server_info->ai_addrlen;
    int packet_counter = 0;
    while (1) {
        memset(block, 0, blocksize);
        int num_bytes_received = recvfrom(sockfd, block, blocksize, 0, server_info->ai_addr, &len);
        if (num_bytes_received <= 0) {
            perror("Client: Error receiving audio data");
            return -1;
        }
        printf("Client: Received packet #%d\n", packet_counter++);

        sem_wait(buffer_sem);
        if (buffer->size + num_bytes_received > buffersize) {
            // Handle overflow situation here
            fprintf(stderr, "Client: Buffer overflow\n");
            return -1;
        } else {
            // Enqueue the received data into the buffer
            for (int i = 0; i < num_bytes_received; i++) {
                enqueue(buffer, block[i]);
            }
        }
        sem_post(buffer_sem);

        // Send feedback packet after each read/write operation
        unsigned short q = prepare_feedback(buffer->size, targetbuf, buffersize);
        if (sendto(sockfd, &q, sizeof(q), 0, server_info->ai_addr, len) == -1) {
            perror("Client: Error sending feedback");
            return -1;
        }
        printf("Client: Sent feedback\n");

        // Record buffer occupancy in the logfile
        // fprintf(logfile, "%d\n", buffer->size);
    }

    // Close
    close(sockfd);
    sem_close(buffer_sem);
    sem_unlink("/buffer_sem");
    destroyQueue(buffer);

}
