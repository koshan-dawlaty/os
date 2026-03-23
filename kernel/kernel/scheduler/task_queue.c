#include "panic.h"
#include <lib/list.h>
#include <scheduler.h>
#include <stdlib.h>
#include <utils.h>

#include <task_queue.h>

extern struct task_queue scheduler_queue;
void task_queue_push(struct task_queue *queue, struct task *t)
{

    /* logd("%p", t->start_addr); */
    /* printf("start:%p\n", t->start_addr); */

    list2c_push(&t->list, &queue->head);
}

struct task *task_queue_pop(struct task_queue *queue)
{

    if (LIST2C_IS_EMPTY(queue->head))
    {
        return NULL;
    }

    /* struct list2c *pos; */
    /* list2c_for_each(pos, &queue->head) */
    /* { */
    /*     printf("%p ", pos); */
    /* } */
    /* printf("\n"); */

    struct list2c *list = list2c_pop(&queue->head);

    if (list == NULL)
    {
        return NULL;
    }
    struct task *out = LIST_ENTRY(list, struct task, list);

    /* the caller might have not done this themselves */
    /* list->next = NULL; */

    return out;
}

void task_queue_remove(struct task_queue *queue, struct task *t)
{
    if (LIST2C_IS_EMPTY(queue->head))
    {
        printf("task queue empty\n");
        return; /* TODO: this is probably an error */
    }

    /* TODO: maybe have a debug assertion to ensure that the task is in the
     * queue */
    list2c_remove(&t->list);

    t->list.next = NULL;
}
