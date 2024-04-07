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
int prepare_feedback(int Q_t, int targetbuf, int buffersize) {
    int q;
    if (Q_t > targetbuf) {
        q = ((Q_t - targetbuf) / (buffersize - targetbuf)) * 10 + 10;
    } else if (Q_t < targetbuf) {
        q = ((targetbuf - Q_t) / targetbuf) * 10;
    } else {
        q = 10;
    }

    return q;
}

// Function to get the size of the file
long get_file_size(FILE *file) {
    long current_position = ftell(file); // Save current position
    fseek(file, 0, SEEK_END); // Seek to the end of the file
    long file_size = ftell(file); // Get the current byte offset in the file
    fseek(file, current_position, SEEK_SET); // Go back to the original position
    return file_size;
}

// Function to calculate the number of packets
int calculate_expected_number_of_packets(const char *filename, int blocksize) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        perror("Error opening file");
        return -1;
    }

    long file_size = get_file_size(file);
    fclose(file);

    int number_of_packets = file_size / blocksize;
    if (file_size % blocksize != 0) {
        number_of_packets++; // Add one more for the remaining bytes
    }

    return number_of_packets;
}
