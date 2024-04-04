#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>

#include "congestion_control.h"
#include "constants.h"

// Function to adjust the sending rate lambda based on feedback
double adjust_sending_rate(double lambda, double epsilon, double gamma, double beta, unsigned short bufferstate) {
    // Calculate adjustment based on method D
    double adjustment = epsilon * (Q_STAR - bufferstate) + beta * (gamma - lambda);

    // Apply adjustment
    lambda += adjustment;

    return lambda;
}

// Function to calculate and send feedback packet
void send_feedback(int sockfd, struct addrinfo *server_info, int Q_t, int targetbuf, int buffersize) {
    int q;
    if (Q_t > targetbuf) {
        q = ((Q_t - targetbuf) / (buffersize - targetbuf)) * 10 + 10;
    } else if (Q_t < targetbuf) {
        q = ((targetbuf - Q_t) / targetbuf) * 10;
    } else {
        q = 10;
    }

    if (sendto(sockfd, &q, sizeof(q), 0, server_info->ai_addr, server_info->ai_addrlen) == -1) {
        perror("Client: Error sending feedback");
    }
}

