#include "lib/log.h"
#include "panic.h"
#include <lib/list.h>
#include <scheduler.h>
#include <stdlib.h>
#include <utils.h>

#include <task_queue.h>

struct task_queue_entry *task_queue_entry_alloc(struct task *task)
{
    struct task_queue_entry *out = malloc(sizeof(struct task_queue_entry));
    out->task = task;
    out->list = LIST_INIT();
    return out;
}

extern struct task_queue scheduler_queue;
void task_queue_push(struct task_queue *queue, struct task *t)
{

    struct task_queue_entry *ent = task_queue_entry_alloc(t);

    /* logd("%p", t->start_addr); */
    /* printf("start:%p\n", t->start_addr); */

    list_push(&ent->list, &queue->head);
}

struct task *task_queue_pop(struct task_queue *queue)
{

    if (LIST_IS_EMPTY(queue->head))
    {
        return NULL;
    }

    struct list *list = list_pop(&queue->head);

    if (list == NULL)
    {
        return NULL;
    }
    struct task_queue_entry *ent =
        LIST_ENTRY(list, struct task_queue_entry, list);
    struct task *out = ent->task;
    free(ent);

    /* the caller might have not done this themselves */
    /* list->next = NULL; */

    return out;
}

void task_queue_remove(struct task_queue *queue, struct task *t)
{
    if (LIST_IS_EMPTY(queue->head))
    {
        return;
    }

    /* TODO: this would be possible to do without a loop if list2c_remove() was
     * used, but the callers of this function would have difficultly working
     * around that */
    struct list *pos;
    list_for_each(pos, &queue->head)
    {
        if (t == TASK_QUEUE_GET_TASK(pos))
        {
            goto found;
        }
    }

    return;

found:

    list_remove(pos);
}
