#ifndef CONCURRENCY_UTILS_H
#define CONCURRENCY_UTILS_H

#include <semaphore.h>

sem_t* create_semaphore(const char* sem_name);

#endif //CONCURRENCY_UTILS_H
