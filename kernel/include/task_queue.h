#ifndef TASK_QUEUE_H
#define TASK_QUEUE_H

#include <lib/list.h>
#include <scheduler.h>

struct task_queue
{
    struct list *head;
};

struct task_queue *task_queue_init();

void task_queue_push(struct task_queue *queue, struct task *t);
struct task *task_queue_pop(struct task_queue *queue);

#endif
