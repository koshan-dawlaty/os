#include "panic.h"
#include <lib/list.h>
#include <scheduler.h>
#include <stdlib.h>
#include <utils.h>

#include <task_queue.h>

extern struct task_queue scheduler_queue;
void task_queue_push(struct task_queue *queue, struct task *t)
{

    if (t->list.next != NULL)
    {
        struct task *next = containerof(t->list.next, struct task, list);
        /* printf("%p\n", next->start_addr); */
    }
    if (queue->head == NULL)
    {
        queue->head = &t->list;
        return;
    }

    list_push(&t->list, &queue->head);
}

struct task *task_queue_pop(struct task_queue *queue)
{
    if (queue->head == NULL)
    {
        return NULL;
    }

    struct list *list = list_pop(&queue->head);
    struct task *out = LIST_ENTRY(list, struct task, list);

    /* the caller might have not done this themselves */
    list->next = NULL;

    return out;
}

void task_queue_remove(struct task_queue *queue, struct task *t)
{
    if (queue->head == NULL)
    {
        printf("task queue empty\n");
        return; /* TODO: this is probably an error */
    }

    if (!list_remove(&t->list, &queue->head))
    {
        printf("task not in task queue\n");
    }

    t->list.next = NULL;
}
