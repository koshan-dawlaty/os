#ifndef WAIT_QUEUE_H
#define WAIT_QUEUE_H

#include <task_queue.h>

struct wait_queue {
struct task_queue queue; /* TODO: if this struct only has one member it could be typedef instead */
};


#define WAIT_QUEUE_INIT() ((struct wait_queue){.queue = TASK_QUEUE_INIT()})


void wait_queue_wait(struct wait_queue*);
void wait_queue_done(struct wait_queue*);

#endif