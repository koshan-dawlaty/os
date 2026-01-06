#include <task_queue.h>


struct wait_queue {
struct task_queue queue; /* TODO: if this struct only has one member it could be typedef instead */
};


#define WAIT_QUEUE_INIT() ((struct wait_queue){.queue = TASK_QUEUE_INIT()})