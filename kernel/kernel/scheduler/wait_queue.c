#include "gdt.h"
#include "panic.h"
#include <container_of.h>
#include <scheduler.h>
#include <task_queue.h>
#include <utils.h>
#include <wait_queue.h>

extern struct task_queue scheduler_queue;
extern struct task *current_task;
extern void print_tasks();

/* FIXME: might be duplicated somewhere */
void wait_queue_wait(struct wait_queue *queue)
{
    preempt_disable();

    struct task *current = get_current_task();

    save_registers(&current->regs);

    task_queue_remove(&scheduler_queue, current);
    if (current == current_task)
    {
        current_task =
            containerof(scheduler_queue.head.next, struct task, list);
        if (&current_task->list == &scheduler_queue.head)
        {
            panic("task queue might be empty!");
            if (current_task == NULL)
            {
                printf("why is this null:%p\n", scheduler_queue.head);
            }
        }
    }

    task_queue_push(&queue->queue, current);

    /* print_tasks(); */

    if (current_task->addr_space.dir != NULL)
    {
        addr_space_load(&current_task->addr_space);
    }
    set_kernel_stack((ptr)current_task->stack);

    /* FIXME: this should maybe be in load_registers() or something */
    preempt_enable();
    load_registers(&current_task->regs);
}

void wait_queue_done(struct wait_queue *queue)
{
    struct task *t = task_queue_pop(&queue->queue);
    if (t != NULL)
    {
        add_task(t);
    }
}
