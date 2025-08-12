#include "scheduler.h"
#include <sched.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct
{
    int value;
    int max_value;

    struct task *queue;
} semaphore;

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
    s->value--;

    if (s->value < 0)
    {
        while (s->value < 0)
        {
            sched_yield();
        }
    }
}
void release_semaphore(semaphore *s)
{
    s->value++;
    while (s->value < 0)
    {
        sched_yield();
    }
}
