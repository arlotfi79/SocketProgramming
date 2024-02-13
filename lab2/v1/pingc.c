/*
** pingc.c -- a datagram "client"
*/

#include "message_utils.h"
#include "socket_utils.h"
#include "constants.h"

// arg[0] = command, arg[1] = server ip, arg[2] = server port, arg[3] = client ip
int main(int argc, char *argv[]) {

    // sockets prep
    int sockfd, rv, numbytes;
    struct addrinfo *server_info, *client_info;
    uint8_t buf[BUF_MAX_LEN];
    memset(buf, 0, BUF_MAX_LEN);


    // timing prep
    struct timeval start_time, end_time;

    // packet prep
    int16_t send_sq_num = 7, recv_sq_num;
    uint8_t send_command = 1, recv_command;


    if (argc < S_ARG_COUNT) {
        fprintf(stderr, "Usage: %s <IPv4 address of destination> <Destination port number> <IPv4 address of this machine>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *const server_ip = argv[1];
    const char *const server_port = argv[2];
    const char *const client_ip = argv[3];
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
    if (bind_socket(client_info, sockfd) == -1)
    {
        if (close(sockfd) == -1) perror("close");
        return -1;
    }

    // Sending message
    encode_packet(buf, &send_sq_num, &send_command);

    gettimeofday(&start_time, NULL);
    if ((numbytes = sendto(sockfd, buf, BUF_MAX_LEN, 0, server_info->ai_addr, server_info->ai_addrlen)) == -1) {
        perror("client: sendto");
        exit(EXIT_FAILURE);
    }
    printf("client: sent %d bytes to %s:%s\n", numbytes, server_ip, server_port);

    // Receiving message
    if ((numbytes = recvfrom(sockfd, buf, BUF_MAX_LEN, 0, NULL, NULL)) == -1) { // Don't care about the address, No need for it when recv
        perror("client: recvfrom");
        exit(EXIT_FAILURE);
    }
    gettimeofday(&end_time, NULL);
    decode_packet(buf, &recv_sq_num, &recv_command);

    if (recv_sq_num == send_sq_num) {
        // Calculate time difference in milliseconds
        long int time_diff_ms = (end_time.tv_sec - start_time.tv_sec) * 1000 +
                                (end_time.tv_usec - start_time.tv_usec) / 1000;

        printf("Time difference: %ld milliseconds\n", time_diff_ms);
    } else
        printf("Sequence numbers do not match");

    // Free
    freeaddrinfo(server_info);
    freeaddrinfo(client_info);
    close(sockfd);

    return 0;
}
