#include "gdt.h"
#include <scheduler.h>
#include <task_queue.h>
#include <utils.h>
#include <wait_queue.h>

void wait_queue_wait(struct wait_queue *queue)
{
    struct task *current = get_current_task();

    /* if (queue->queue.head == NULL) */
    /* { */
    /*     queue->queue.head = &current->list; */
    /*     return; */
    /* } */

    /* list_push(&current->list, &queue->queue.head); */
    IRQ_OFF({
        task_queue_add_current(&queue->queue);
        /* task_queue_push(&queue->queue, current); */
    )};

	current = get_current_task();
    if (current->addr_space.dir != NULL)
    {
        addr_space_load(current->addr_space);
    }
    set_kernel_stack((ptr)current->stack);
    load_registers(&current->regs);

    /* remove_task(current); /1* TODO: this might cause problems because it is
     * run with irqs on, the saved register state could maybe change in between
     * these. Right now it is run with irqs on because it jumps to other code
     * and that code needs irqs on *1/ */
}

void wait_queue_done(struct wait_queue *queue)
{
    add_task(task_queue_pop(&queue->queue));
}
