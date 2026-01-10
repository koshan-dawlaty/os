#include <scheduler.h>
#include <task_queue.h>
#include <utils.h>
#include <wait_queue.h>

void wait_queue_wait(struct wait_queue *queue)
{
    struct task *current = get_current_task();
    IRQ_OFF({
        task_queue_push(&queue->queue, current);
        save_registers(&current->regs);
    )};
    remove_task(current); /* TODO: this might cause problems because it is run with irqs on, the saved register state could maybe change in between these. Right now it is run with irqs on because it jumps to other code and that code needs irqs on */
}

void wait_queue_done(struct wait_queue *queue)
{
    add_task(task_queue_pop(&queue->queue));
}
