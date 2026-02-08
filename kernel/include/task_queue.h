
#ifndef TASK_QUEUE_H
#define TASK_QUEUE_H

#include <lib/list.h>
struct task;

struct task_queue
{
    struct list *head;
};

#define TASK_QUEUE_INIT()                                                      \
    (struct task_queue)                                                        \
    {                                                                          \
        .head = NULL                                                           \
    }

void task_queue_push(struct task_queue *queue, struct task *t);
struct task *task_queue_pop(struct task_queue *queue);
void task_queue_remove(struct task_queue *queue, struct task *t);

#endif
