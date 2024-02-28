#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>


#include "ack_handling.h"
#include "constants.h"


volatile sig_atomic_t alarm_flag = 0;

// SIGALRM handler
void alarm_handler(int signum) {
    alarm_flag = 1;
}

// Initialize packet tracker
void init_packet_tracker(PacketTracker *tracker, int status) {
    memset(tracker->received, status, sizeof(tracker->received));
}

// Check if packet is missing
int packet_status(PacketTracker *tracker, int sequence_number) {
    return tracker->received[sequence_number];
}

void mark_packet(PacketTracker *tracker, int sequence_number, int status) {
    printf("mark: Received packet#%d\n", sequence_number);
    tracker->received[sequence_number] = status;
}

double check_transfer_success_rate (PacketTracker *tracker, int number_of_packets) {
    int count = 0;
    double rate = 0.0;

    for (int i = 0; i < number_of_packets + 1; ++i) {
        if (tracker->received[i] == NOT_RECEIVED)
            count++;
    }

    rate = (count / number_of_packets) * 10;

    return rate;
}

