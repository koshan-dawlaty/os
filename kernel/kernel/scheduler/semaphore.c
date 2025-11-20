#include "scheduler.h"
#include <kernel/semaphore.h>
#include <panic.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

semaphore *new_semaphore(int max_value)
{
    semaphore *out = malloc(sizeof(semaphore));
    out->value = max_value;
    out->queue = NULL;

    return out;
}

void wait_semaphore(semaphore *s)
{

    asm("cli");

    s->value--;
    /* printf("%d\n", s->value); */

    if (s->value < 0)
    {
        struct task *current = get_current_task();
        remove_task(current);
        struct task *temp = s->queue;
        s->queue = current;
        s->queue->next = temp;
        /* add_task_queue(&s->queue, current); */

        sched_yield();
    }

    asm("sti");
}
void release_semaphore(semaphore *s)
{
    asm("cli");

    s->value++;

    /* printf("%d\n", s->value); */

    if (s->value <= 0)
    {

        struct task *temp = s->queue;
        s->queue = temp->next;

        /* remove_task_queue(&s->queue, task); */

        add_task(temp);
    }

    asm("sti");
}

void free_semaphore(semaphore *s)
{
    /* TODO: maybe free the task queue */

    free(s);
}
