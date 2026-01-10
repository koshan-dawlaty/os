#include <scheduler.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <task_queue.h>
#ifndef MUTEX_H
#define MUTEX_H

struct mutex
{
    volatile bool locked;
    struct task_queue queue;
};

#define MUTEX_INIT()                                                           \
    (struct mutex)                                                             \
    {                                                                          \
        .locked = false, .queue = TASK_QUEUE_INIT()                            \
    }

void mutex_lock(struct mutex *m);
void mutex_unlock(struct mutex *m);

#endif
