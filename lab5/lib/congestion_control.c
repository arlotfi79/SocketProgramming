#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>

#include "congestion_control.h"
#include "constants.h"

// Function to adjust the sending rate lambda based on feedback
double adjust_sending_rate(double lambda, double epsilon, double gamma, double beta, unsigned short bufferstate) {
    // Calculate adjustment based on method D if CONTROLLAW is 0, on method C if it is 1
    double adjustment = epsilon * (Q_STAR - bufferstate) + (1-CONTROLLAW) * beta * (gamma - lambda);

    // Apply adjustment
    lambda += adjustment;

    return lambda;
}

// Function to calculate and send feedback packet
int prepare_feedback(int Q_t, int targetbuf, int buffersize) {
    int q;
    if (Q_t > targetbuf) {
        q = ((Q_t - targetbuf) / (buffersize - targetbuf)) * 10 + 10;
    } else if (Q_t < targetbuf) {
        q = 10 - (((targetbuf - Q_t) / targetbuf) * 10);
    } else {
        q = 10;
    }

    return q;
}



