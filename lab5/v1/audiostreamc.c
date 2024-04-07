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

sem_t *buffer_sem;
struct timeval start_time;

// TODO -> Client: implement reading audio data from the buffer and writing it to the audio device
// Run -> ./audiostreamc.bin kj.au 4096 2899968 4096 127.0.0.1 5000 logFileC
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

    // TODO -> Client: adding 313 msec signal to handle audio reading!
//    struct itimerval timer = {{0, 313000}, {0, 1}};
//    setitimer(ITIMER_REAL, &timer, NULL);
//    gettimeofday(&start_time, NULL);

    // Loop for receiving audio data
    Queue *buffer = createQueue(buffersize);
    uint8_t block[blocksize];
    socklen_t len = server_info->ai_addrlen;
    int packet_counter = 0;

    // alarm for not receiving 1st response from the server
    struct timeval timeout;
    timeout.tv_sec = 2;  // after 2 seconds
    timeout.tv_usec = 0;

    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("Error");
        return -1;
    }

    // Get the process ID
    pid_t pid = getpid();

    // Create the log file name
    char logfile_name[100];
    sprintf(logfile_name, "%s-%d.csv", logfileC, pid);

    // Open the log file
    FILE *log_file = fopen(logfile_name, "w");
    if (log_file == NULL) {
        perror("Error opening log file");
        exit(EXIT_FAILURE);
    }

    fprintf(log_file, "%s, %s\n", "time (ms)", "buffer_size");

    while (1) {
        memset(block, 0, blocksize);
        int num_bytes_received = recvfrom(sockfd, block, blocksize, 0, server_info->ai_addr, &len);
        if (num_bytes_received <= 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                printf("Timeout occurred\n");
                return -1;
            } else {
                perror("Client: Error receiving audio data");
                return -1;
            }
        }
        printf("Client: Received packet #%d\n", packet_counter++);

        // TODO: fix buffer overflow that occurs here (I assume bec of buffer size as we are supposed to play the audio file in real time)
        int result = handle_received_data(buffer, block, num_bytes_received, buffer_sem, buffersize);
        if (result == -1) {
            perror("Client: Error handling received data (semaphore error)");
            return -1;
        }

        // Send feedback packet after each read/write operation
        unsigned short q = prepare_feedback(buffer->size, targetbuf, buffersize);
        if (sendto(sockfd, &q, sizeof(q), 0, server_info->ai_addr, len) == -1) {
            perror("Client: Error sending feedback");
            return -1;
        }
        printf("Client: Sent feedback\n");

        // Log the buffer size and normalized time
        struct timeval current_time;
        gettimeofday(&current_time, NULL);
        double normalized_time = (current_time.tv_sec - start_time.tv_sec) * 1000.0 + (current_time.tv_usec - start_time.tv_usec) / 1000.0;

        fprintf(log_file, "%.3f, %d\n", normalized_time, buffer->size);
    }


    // Close
    close(sockfd);
    fclose(log_file);
    sem_close(buffer_sem);
    sem_unlink("/buffer_sem");
    destroyQueue(buffer);

}
