#include <semaphore.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "concurrency_utils.h"

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
