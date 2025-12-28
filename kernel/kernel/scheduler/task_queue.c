#include <lib/list.h>
#include <scheduler.h>
#include <stdlib.h>
#include <utils.h>

#include <task_queue.h>

void task_queue_push(struct task_queue *queue, struct task *t)
{
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
}
