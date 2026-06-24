
#ifndef TASK_QUEUE_H
#define TASK_QUEUE_H

#include <lib/list.h>
struct task;

struct task_queue
{
    struct list head;
};

struct task_queue_entry
{
    struct task *task;
    struct list list;
};

#define TASK_QUEUE_INIT(name)                                                  \
    (struct task_queue)                                                        \
    {                                                                          \
        .head = LIST2C_HEAD_INIT(&(name).head)                                 \
    }

#define TASK_QUEUE_DEFINE(name) struct task_queue name = TASK_QUEUE_INIT(name)

static inline void task_queue_init(struct task_queue *q)
{
    *q = TASK_QUEUE_INIT(*q);
}

#define TASK_QUEUE_GET_TASK(ptr)                                               \
    (containerof(ptr, struct task_queue_entry, list)->task)

void task_queue_push(struct task_queue *queue, struct task *t);
struct task *task_queue_pop(struct task_queue *queue);
void task_queue_remove(struct task_queue *queue, struct task *t);

struct task_queue_entry *task_queue_entry_alloc(struct task *task);

#endif
