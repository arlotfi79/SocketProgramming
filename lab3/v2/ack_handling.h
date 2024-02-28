#ifndef ACK_HANDLING_H
#define ACK_HANDLING_H

#include <signal.h>

#define MAX_SEQ_NUM 255
#define RETRANSMIT_TIMEOUT 100 // 100 milliseconds in microseconds
#define NOT_RECEIVED 0
#define RECEIVED 1


// Structure to track received packets
typedef struct {
    int received[MAX_SEQ_NUM + 1];
} PacketTracker;


extern volatile sig_atomic_t alarm_flag; // Declare alarm_flag variable
extern volatile int transit_mode;

void alarm_handler(int signum);

void nack_handler(int signum);

void init_packet_tracker(PacketTracker *tracker, int status);

int packet_status(PacketTracker *tracker, int sequence_number);

void mark_packet(PacketTracker *tracker, int sequence_number, int status);

double check_transfer_success_rate (PacketTracker *tracker, int segment_size);


#endif // ACK_HANDLING_H
