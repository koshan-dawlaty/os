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

#define MUTEX_INIT(name)                                                       \
    (struct mutex)                                                             \
    {                                                                          \
        .locked = false, .queue = TASK_QUEUE_INIT((name).queue)                \
    }

/* TODO: the name maybe shouldnt be as close to the macro above this,
 * MUTEX_INIT(), but theres maybe not a better name that still has the
 * typename_functioname naming scheme */
static inline void mutex_init(struct mutex *m)
{
    *m = MUTEX_INIT(*m);
}

#define MUTEX_DEFINE(name) struct mutex name = MUTEX_INIT(name)

void mutex_lock(struct mutex *m);
void mutex_unlock(struct mutex *m);

#endif
