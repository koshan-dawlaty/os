#ifndef SEMAPHORE
#define SEMAPHORE

#include "spinlock.h"
#include "task_queue.h"

typedef struct semaphore
{
    int value;

    struct task_queue queue;
} semaphore;

struct semaphore *new_semaphore(int max_value);

void wait_semaphore(struct semaphore *s);
void release_semaphore(struct semaphore *s);

void free_semaphore(struct semaphore *s);

#endif
