#ifndef CONCURRENCY_UTILS_H
#define CONCURRENCY_UTILS_H

#include <semaphore.h>
#include <fcntl.h>

#include "queue.h"

void handle_alarm(int sig);

sem_t* create_semaphore(const char* sem_name);
int handle_received_data(Queue* buffer, uint8_t* block, int num_bytes_received, sem_t* buffer_sem, int buffersize);

#endif //CONCURRENCY_UTILS_H
