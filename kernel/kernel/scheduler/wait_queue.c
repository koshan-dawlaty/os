#include "gdt.h"
#include <scheduler.h>
#include <task_queue.h>
#include <utils.h>
#include <wait_queue.h>

extern struct task_queue scheduler_queue;
extern struct task *current_task;

void wait_queue_wait(struct wait_queue *queue)
{
    irq_state_t state = irq_save_disable();
    struct task *current = get_current_task();

    task_queue_push(&queue->queue, current);
    save_registers(&current->regs);

    task_queue_remove(&scheduler_queue, current);

    if (current == current_task)
    {
        current_task = LIST_ENTRY(current_task->list.next, struct task, list);
        if (current_task == NULL)
        {
            current_task = LIST_ENTRY(scheduler_queue.head, struct task, list);

            if (current_task == NULL)
            {
                printf("why is this null:%p\n", scheduler_queue.head);
            }
        }
    }

    if (current_task->addr_space.dir != NULL)
    {
        addr_space_load(current_task->addr_space);
    }
    set_kernel_stack((ptr)current_task->stack);
    load_registers(&current_task->regs);
    irq_restore(state);
}

void wait_queue_done(struct wait_queue *queue)
{
    struct task *t = task_queue_pop(&queue->queue);
    add_task(t);
}
