#ifndef WAIT_QUEUE_H
#define WAIT_QUEUE_H

#include <stdlib.h>
#include <task_queue.h>

struct wait_queue
{
    struct task_queue queue; /* TODO: if this struct only has one member it
                                could be typedef instead */
};

#define WAIT_QUEUE_INIT() ((struct wait_queue){.queue = TASK_QUEUE_INIT()})

#define WAIT_QUEUE_EMPTY(q) ((q).queue.head == NULL)

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
