#include <task_queue.h>
#include <scheduler.h>
#include <utils.h>


struct wait_queue {
struct task_queue queue; /* TODO: if this struct only has one member it could be typedef instead */
};


#define WAIT_QUEUE_INIT() ((struct wait_queue){.queue = TASK_QUEUE_INIT()})


void wait_queue_wait(struct wait_queue * queue) {
    IRQ_OFF({
        struct task *current = get_current_task();
        task_queue_push(&queue->queue, current);
        save_registers(&current->regs);
        remove_task(current); /* TODO: maybe this shouldnt be in the IRQ_OFF block */
    });
}

void wait_queue_done(struct wait_queue * queue) {
    add_task(task_queue_pop(&queue->queue));
}
