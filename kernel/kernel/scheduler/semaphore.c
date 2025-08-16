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
    out->value = 1;
    out->max_value = max_value;
    out->queue = NULL;

    return out;
}

void wait_semaphore(semaphore *s)
{

    asm("cli");

    s->value--;
    printf("%d\n", s->value);

    if (s->value < 0)
    {
        struct task *current = get_current_task();
        remove_task(current);
        add_task_queue(&s->queue, current);
        printf("first in queue:%p\n", s->queue);
        sched_yield();
    }
    asm("sti");
}
void release_semaphore(semaphore *s)
{
    asm("cli");
    s->value++;

    printf("%d\n", s->value);

    if (s->value <= 0)
    {
        struct task *task = s->queue;
        remove_task_queue(&s->queue, task);
        add_task(task);
    }
    asm("sti");
}
