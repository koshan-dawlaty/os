#ifndef WAIT_QUEUE_H
#define WAIT_QUEUE_H

#include <lib/list.h>
#include <stdlib.h>
#include <task_queue.h>

struct wait_queue
{
    struct task_queue queue; /* TODO: if this struct only has one member it
                                could be typedef instead */
};

#define WAIT_QUEUE_INIT(name)                                                  \
    ((struct wait_queue){.queue = TASK_QUEUE_INIT((name).queue)})

/* TODO: the name maybe shouldnt be as close to the macro above this,
 * WAIT_QUEUE_INIT(), but theres maybe not a better name that still has the
 * typename_functioname naming scheme */
static inline void wait_queue_init(struct wait_queue *queue)
{
    *queue = WAIT_QUEUE_INIT(*queue);
}

#define WAIT_QUEUE_DEFINE(name) struct wait_queue name = WAIT_QUEUE_INIT(name)

#define WAIT_QUEUE_EMPTY(q) (LIST2C_IS_EMPTY((q).queue.head))

/*
 * adds the current task to the queue
 * the currently running task will stop running until it is removed from the
 * queue (after one or more calls to wait_queue_done())
 */
void wait_queue_wait(struct wait_queue *);
/*
 * removes a task from the queue and makes it run again
 */
void wait_queue_done(struct wait_queue *);

static inline void wait_queue_wake_all(struct wait_queue *queue)
{
    while (!WAIT_QUEUE_EMPTY(*queue))
    {
        wait_queue_done(queue);
    }
}

#endif
