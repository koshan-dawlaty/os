#include "sleep.h"
#include "container_of.h"
#include "lib/list.h"
#include "scheduler.h"
#include "utils.h"
#include <stdlib.h>

list2 *sleep_queue_head = NULL;

void add_sleep_queue(struct task *task, size_t time)
{
    irq_disable();

    struct sleep_queue_entry *new = malloc(sizeof(struct sleep_queue_entry));

    new->task = task;
    new->ticks = time;

    if (sleep_queue_head == NULL)
    {
        sleep_queue_head = &new->l;
    }
    else
    {
        list2_add(&new->l, sleep_queue_head);
    }

    remove_task(task);

    irq_enable();
    schedule();
}

void increment_sleep_queue()
{
    struct sleep_queue_entry *head =
        LIST_ENTRY(sleep_queue_head, struct sleep_queue_entry, l);

    if (head->task == NULL)
    {
        return;
    }

    head->ticks--;

    printf("%d\n", head->ticks);
    if (head->ticks == 0)
    {

        printf("head:%p\n", sleep_queue_head);
        printf("next:%p\n", sleep_queue_head->next);

        /* MIGHT NOT WORK */
        list2_remove(sleep_queue_head, &sleep_queue_head);
        add_task(head->task);

        printf("after:%p\n", sleep_queue_head);
        if (sleep_queue_head != NULL)
        {
            printf("next:%p\n\n", sleep_queue_head->next);
        }

        free(head);
    }
}
