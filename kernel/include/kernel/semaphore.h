#ifndef SEMAPHORE
#define SEMAPHORE

#include "spinlock.h"
typedef struct
{
    int value;

    struct task *queue;
} semaphore;

semaphore *new_semaphore(int max_value);

void wait_semaphore(semaphore *s);
void release_semaphore(semaphore *s);

void free_semaphore(semaphore *s);

#endif
