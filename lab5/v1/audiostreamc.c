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
#include <fcntl.h>
#include <signal.h>
#include <time.h>


#include "../lib/constants.h"
#include "../lib/socket_utils.h"
#include "../lib/congestion_control.h"
#include "../lib/concurrency_utils.h"

sem_t *buffer_sem;
struct timeval start_time;
// int buffer_occupancy = 0;
int targetbuf;
int sockfd, rv;
struct addrinfo *server_info, *client_info;
socklen_t len;
int buffersize;
Queue *buffer;



// Signal handler function called to play the audio and empty buffer
void buffer_read_handler(int sig) {
    static uint8_t data[READ_SIZE];
    sem_wait(buffer_sem);
    if (buffer->size > READ_SIZE) {
        size_t bytesRead = 0;
        while (bytesRead < READ_SIZE) {
            int item = dequeue(buffer);
            if (item == -1) {
                break;  // Queue is empty, stop reading
            }
            data[bytesRead++] = (uint8_t)item;  // Cast the int to uint8_t before storing
            
        }
        // TODO add this function
        // mulawwrite(data, READ_SIZE);
        printf("Client: buffer read after 313 milliseconds\n");
        // Send feedback packet after each read/write operation
        unsigned short q = prepare_feedback(buffer->size, targetbuf, buffersize);
        sem_post(buffer_sem);
        if (sendto(sockfd, &q, sizeof(q), 0, server_info->ai_addr, len) == -1) {
            perror("Client: Error sending feedback");
            exit(-1);
        }
        printf("Client: Sent feedback after reading\n");
    }else{
       sem_post(buffer_sem); 
    }


}

// Setup timer and signal handler for periodic execution
void setup_periodic_timer() {
    struct sigaction sa;
    struct sigevent sev;
    timer_t timerid;
    struct itimerspec its;

    // Set up signal handler
    sa.sa_flags = SA_SIGINFO | SA_RESTART;
    sa.sa_handler = buffer_read_handler;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGRTMIN, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    // Create the timer
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGRTMIN;
    if (timer_create(CLOCK_REALTIME, &sev, &timerid) == -1) {
        perror("timer_create");
        exit(EXIT_FAILURE);
    }

    // Start the timer to fire every 313 milliseconds
    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = 313000000; // 313 milliseconds in nanoseconds
    its.it_interval.tv_sec = its.it_value.tv_sec;
    its.it_interval.tv_nsec = its.it_value.tv_nsec;

    if (timer_settime(timerid, 0, &its, NULL) == -1) {
        perror("timer_settime");
        exit(EXIT_FAILURE);
    }
}



// Run -> ./audiostreamc.bin kj.au 4096 2899968 4096 127.0.0.1 5000 logFileC
int main(int argc, char *argv[]) {
    if (argc != C_ARG_COUNT) {
        fprintf(stderr, "Usage: %s <audiofile> <blocksize> <buffersize> <targetbuf> "
                        "<server-IP> <server-port> <logfileC>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *audiofile = argv[1];
    int blocksize = atoi(argv[2]);
    buffersize = atoi(argv[3]);
    targetbuf = atoi(argv[4]);
    const char *server_ip = argv[5];
    const char *server_port = argv[6];
    const char *logfileC = argv[7];



    // sockets prep
    // int sockfd, rv;
    // struct addrinfo *server_info, *client_info;

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
    buffer = createQueue(buffersize);
    uint8_t block[blocksize];
    // unsigned int len = sizeof(server_info);
    len = server_info->ai_addrlen;
    int packet_counter = 0;

    // alarm for not receiving 1st response from the server
    // TODO is this necesary? handout does not mention this.
    // struct timeval timeout;
    // timeout.tv_sec = 2;  // after 2 seconds
    // timeout.tv_usec = 0;

    // if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
    //     perror("Error");
    //     return -1;
    // }

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

    setup_periodic_timer();

    while (1) {
        memset(block, 0, blocksize);
        int num_bytes_received = recvfrom(sockfd, block, blocksize, 0, server_info->ai_addr, &len);
        if (num_bytes_received <= 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                printf("Timeout occurred\n");
                return -1;
            } else {
                if (num_bytes_received == 0) {
                    //TODO if number of bytes is zero 5 times connection should close and end
                    printf("0 bytes recieved\n");
                }else{
                    perror("Client: Error receiving audio data");
                    return -1;
                }
            
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
        printf("Client: q in client is %d\n", q);
        if (sendto(sockfd, &q, sizeof(q), 0, server_info->ai_addr, len) == -1) {
            perror("Client: Error sending feedback");
            return -1;
        }
        printf("Client: Sent feedback %d\n", q);
        
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
