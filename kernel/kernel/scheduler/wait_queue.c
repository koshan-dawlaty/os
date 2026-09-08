#include "gdt.h"
#include "lib/log.h"
#include <container_of.h>
#include <lib/assert.h>
#include <scheduler.h>
#include <task_queue.h>
#include <utils.h>
#include <wait_queue.h>

extern struct sched_queue scheduler_queue;
extern void print_tasks();

/* FIXME: might be duplicated somewhere */
void wait_queue_wait(struct wait_queue *queue)
{
    preempt_disable();

    struct task *current = get_current_task();

    ASSERT(current->can_sleep, "current task can't sleep");

    struct task *old = current;
    /* save_registers(&current->regs); */

    task_queue_remove(&scheduler_queue.queue, current);
    if (current == SCHED_QUEUE_CURRENT_TASK(scheduler_queue))
    {

        struct list *list = scheduler_queue.queue.head.next;
        scheduler_queue.current_task =
            containerof(list, struct task_queue_entry, list);

        if (&SCHED_QUEUE_CURRENT_TASK(scheduler_queue)->list ==
            &scheduler_queue.queue.head)
        {
            panic("task queue might be empty!");
            if (SCHED_QUEUE_CURRENT_TASK(scheduler_queue) == NULL)
            {
                printf("why is this null:%p\n", scheduler_queue.queue.head);
            }
        }
    }

    task_queue_push(&queue->queue, current);

    /* print_tasks(); */

    if (SCHED_QUEUE_CURRENT_TASK(scheduler_queue)->addr_space->dir != NULL)
    {
        addr_space_load(SCHED_QUEUE_CURRENT_TASK(scheduler_queue)->addr_space);
    }
    set_kernel_stack((ptr_t)SCHED_QUEUE_CURRENT_TASK(scheduler_queue)->stack);

    /* FIXME: this should maybe be in load_registers() or something */
    preempt_enable();
    switch_to(old, SCHED_QUEUE_CURRENT_TASK(scheduler_queue));
}

void wait_queue_done(struct wait_queue *queue)
{
    struct task *t = task_queue_pop(&queue->queue);

    ASSERT(t->can_sleep, "attempt to add a task that can't sleep");
    if (t != NULL)
    {
        add_task(t);
    }
}
