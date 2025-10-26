#ifndef SCHED_SLEEP_H
#define SCHED_SLEEP_H

#include "lib/list.h"
#include <stddef.h>

struct sleep_queue_entry
{
    struct task *task;
    /* difference in number of ticks between this one and the next one  */
    size_t ticks;
    list2 l;
};

/*
 * increments the head of the sleep queue
 */
void increment_sleep_queue();

#endif
