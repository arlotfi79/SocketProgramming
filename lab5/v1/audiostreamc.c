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


// -----------------------------
#include <alsa/asoundlib.h>
/* audio codec library functions */

static snd_pcm_t *mulawdev;
static snd_pcm_uframes_t mulawfrms;

// Initialize audio device.
void mulawopen(size_t *bufsiz) {
    snd_pcm_hw_params_t *p;
    unsigned int rate = 8000;

    snd_pcm_open(&mulawdev, "default", SND_PCM_STREAM_PLAYBACK, 0);
    snd_pcm_hw_params_alloca(&p);
    snd_pcm_hw_params_any(mulawdev, p);
    snd_pcm_hw_params_set_access(mulawdev, p, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(mulawdev, p, SND_PCM_FORMAT_MU_LAW);
    snd_pcm_hw_params_set_channels(mulawdev, p, 1);
    snd_pcm_hw_params_set_rate_near(mulawdev, p, &rate, 0);
    snd_pcm_hw_params(mulawdev, p);
    snd_pcm_hw_params_get_period_size(p, &mulawfrms, 0);
    *bufsiz = (size_t)mulawfrms;
    return;
}

// Write to audio device.
#define mulawwrite(x) snd_pcm_writei(mulawdev, x, mulawfrms)

// Close audio device.
void mulawclose(void) {
    snd_pcm_drain(mulawdev);
    snd_pcm_close(mulawdev);
}

// -----------------------------

sem_t *buffer_sem;
struct timeval start_time;
// int buffer_occupancy = 0;
int targetbuf;
int sockfd, rv;
struct addrinfo *server_info, *client_info;
socklen_t len;
int buffersize;
Queue *buffer;
volatile int initial_recieved;

void two_alarm_handler(int sig) {
    if (initial_recieved == 0) {
        printf("No packet recieved for 2 seconds.\n");
        printf("Exiting program.\n");
        exit(0);
    } else {
        printf("Packet recieved in 2 seconds, not exiting program\n");
    }
}



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
        mulawwrite(data);
        printf("Client: buffer read after 313 milliseconds\n");
        sem_post(buffer_sem);
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



// Run -> ./audiostreamc.bin kj.au 4096 65536 32768 127.0.0.1 5000 logFileC
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

    // Loop for receiving audio data
    buffer = createQueue(buffersize);
    uint8_t block[blocksize];
    // unsigned int len = sizeof(server_info);
    len = server_info->ai_addrlen;
    int packet_counter = 0;

    // alarm for not receiving 1st response from the server
    initial_recieved = 0;
    signal(SIGALRM, two_alarm_handler);
    // Set alarm to go off after 2 seconds
    alarm(2);


    // Get the process IDinitial_recieved
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
    size_t bufsiz;
    mulawopen(&bufsiz);

    int empty_packets_recieved = 0;
    while (1) {
        memset(block, 0, blocksize);
        int num_bytes_received = recvfrom(sockfd, block, blocksize, 0, server_info->ai_addr, &len);
        alarm(0);
        initial_recieved = 1;
        if (num_bytes_received <= 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                printf("Timeout occurred\n");
                return -1;
            } else {
                if (num_bytes_received == 0) {
                    empty_packets_recieved ++;
                    if (empty_packets_recieved == 5){
                        printf("Client: five empty packets recieved, ending transmission\n");
                        break;
                    }
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
        printf("-----------------------------\n");
    }


    // Close
    close(sockfd);
    fclose(log_file);
    sem_close(buffer_sem);
    sem_unlink("/buffer_sem");
    destroyQueue(buffer);

}

