/*
** ssftpd.c -- a datagram sockets "server" for small size file transfer protocol
*/

#include "message_utils.h"
#include "socket_utils.h"
#include "constants.h"

// arg[0] = command, arg[1] = ip, arg[2] = port
int main(int argc, char *argv[]) {
    if (argc < S_ARG_COUNT) {
        fprintf(stderr, "Usage: %s <IPv4 address> <port number>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *const ip = argv[1];
    const char *const initial_port = argv[2];
    char port[MAX_PORT_NUM];


    int sockfd, rv, numbytes, attempts = 0;
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


    printf("Server: successfully bound to port %s\n", port);
    freeaddrinfo(servinfo);


    while (1) {
        printf("Server: waiting to recvfrom...\n");

        char filename[MAX_FILENAME_LENGTH + 1]; // +1 for null terminator
        memset(filename, '\0', sizeof(filename));

        addr_len = sizeof client_addr;
        if ((numbytes = recvfrom(sockfd, filename, MAX_FILENAME_LENGTH, 0, (struct sockaddr *) &client_addr,
                                 &addr_len)) == -1) {
            perror("recvfrom");
            close(sockfd);
            exit(EXIT_FAILURE);
        }
        if (numbytes == 0)
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
        FILE *file = fopen(filename, "rb");
        if (file == NULL) {
            fflush(stdout);
            printf("ignoring request");
            fprintf(stderr, "File does not exist: %s\n", filename);
            continue;
        }

        // Get file size
        fseek(file, 0L, SEEK_END); // move the file position indicator to the end of the file
        long file_size = ftell(file); // returns the current value of the file position indicator
        rewind(file); // reset the file position indicator to the beginning of the file

        // Read file content into memory
        char *file_content = (char *) malloc(file_size);
        if (file_content == NULL) {
            perror("malloc");
            fclose(file);
            continue;
        }
        fread(file_content, 1, file_size, file);
        fclose(file);

        // Send file size to client
        char file_size_packet[SERVER_FIRST_PCK_SIZE];
        memset(file_size_packet, 0, sizeof(file_size_packet));
        file_size_packet[0] = (file_size >> 16) & 0xFF;
        file_size_packet[1] = (file_size >> 8) & 0xFF;
        file_size_packet[2] = file_size & 0xFF;
        if (sendto(sockfd, file_size_packet, SERVER_FIRST_PCK_SIZE, 0, (struct sockaddr *) &client_addr, addr_len) ==
            -1) {
            perror("sendto");
            free(file_content);
            continue;
        }

        // Send file content to client in chunks
        int sequence_number = 0;
        int bytes_sent = 0;
        while (bytes_sent < file_size) {
            int bytes_to_send = (file_size - bytes_sent > SERVER_MAX_PAYLOAD_SIZE) ? SERVER_MAX_PAYLOAD_SIZE : (
                    file_size - bytes_sent);
            char buf[SERVER_MAX_PAYLOAD_SIZE + 1];

            buf[0] = sequence_number++;
            memcpy(buf + 1, file_content + bytes_sent, bytes_to_send);

            if (sendto(sockfd, buf, bytes_to_send + 1, 0, (struct sockaddr *) &client_addr, addr_len) == -1) {
                perror("sendto");
                free(file_content);
                continue;
            }
            bytes_sent += bytes_to_send;
        }

        free(file_content);
    }
}