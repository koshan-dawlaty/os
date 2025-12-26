#ifndef MUTEX_H
#define MUTEX_H

#include <stdatomic.h>
#include <stdbool.h>

struct mutex
{
    volatile bool locked;
    struct task *queue;
};

#define MUTEX_INIT()                                                           \
    (struct mutex)                                                             \
    {                                                                          \
        .locked = false                                                        \
    }

void mutex_lock(struct mutex *m);
void mutex_unlock(struct mutex *m);

#endif
