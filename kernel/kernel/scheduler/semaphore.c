/* TODO: this is currently broken */
#include "assert.h"
#include "scheduler.h"
#include "task_queue.h"
#include <kernel/semaphore.h>
#include <lib/log.h>
#include <panic.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

struct semaphore *new_semaphore(int max_value)
{
    struct semaphore *out = malloc(sizeof(struct semaphore));
    out->value = max_value;
    task_queue_init(&out->queue);

    return out;
}

void wait_semaphore(semaphore *s)
{

    preempt_disable();

    s->value--;
    /* printf("%d\n", s->value); */

    if (s->value < 0)
    {
        logd("removing task");
        /* s->queue->next = temp; */
        /* add_task_queue(&s->queue, current); */

        struct task *current = get_current_task();

        save_registers(&current->regs);
        remove_task_add_queue(current, &s->queue);
    }

    preempt_enable();
}
void release_semaphore(semaphore *s)
{
    preempt_disable();

    s->value++;

    /* printf("%d\n", s->value); */

    if (s->value <= 0)
    {

        logd("adding task");

        struct task *temp = task_queue_pop(&s->queue);
        ASSERT(temp != NULL, "semaphore contended when task queue emtpy");

        add_task(temp);
        schedule();
        /* s->queue = temp->next; */

        /* remove_task_queue(&s->queue, task); */

        add_task(temp);
    }

    preempt_enable();
}

void free_semaphore(semaphore *s)
{
    /* TODO: maybe free the task queue */

    free(s);
}
