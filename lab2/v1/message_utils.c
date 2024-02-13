#include "message_utils.h"

void encode_packet(uint8_t *buffer, const int16_t *sequence_number, const uint8_t *command) {
    // Assign sequence number in network byte order (big-endian)
    *(int16_t *) &buffer[0] = htons(*sequence_number);

    // Assign command
    buffer[2] = *command;
}

void decode_packet(const uint8_t *buffer, int16_t *sequence_number, uint8_t *command) {
    // Extract sequence number (16 bits, big-endian)
    *sequence_number = (buffer[0] << 8) | buffer[1];

    // Extract command (8 bits)
    *command = buffer[2];
}
