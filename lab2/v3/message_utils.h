#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>

void encode_packet(uint8_t *buffer, const int16_t *sequence_number, const uint8_t *command);
void decode_packet(const uint8_t *buffer, int16_t *sequence_number, uint8_t *command);

#endif //UTILS_H
