#include <semaphore.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include "concurrency_utils.h"
#include "queue.h"

void handle_alarm(int sig) {
    printf("No data received for 2 seconds. Exiting...\n");
    exit(EXIT_FAILURE);
}

sem_t* create_semaphore(const char* sem_name) {
    sem_t* buffer_sem = sem_open(sem_name, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1);
    if (buffer_sem == SEM_FAILED) {
        if (errno == EEXIST) {
            buffer_sem = sem_open(sem_name, 0);
            sem_unlink(sem_name);
            buffer_sem = sem_open(sem_name, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1);
            if (buffer_sem == SEM_FAILED) {
                perror("Client:sem_open: Error recreating semaphore\n");
                exit(EXIT_FAILURE);
            }
        } else {
            perror("Client:sem_open: Error creating semaphore\n");
            exit(EXIT_FAILURE);
        }
    }
    printf("Client: Semaphore created successfully\n");
    return buffer_sem;
}

int handle_received_data(Queue* buffer, uint8_t* block, int num_bytes_received, sem_t* buffer_sem, int buffersize) {
    sem_wait(buffer_sem);
    if (buffer->size + num_bytes_received > buffersize) {
        // Handle overflow situation here
        fprintf(stderr, "Client: Buffer overflow\n");
        sem_post(buffer_sem);
        return -1;
    } else {
        // Enqueue the received data into the buffer
        for (int i = 0; i < num_bytes_received; i++) {
            enqueue(buffer, block[i]);
        }
    }
    sem_post(buffer_sem);
    return 0;
}
