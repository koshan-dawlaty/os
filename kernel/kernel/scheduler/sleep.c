#include "sleep.h"
#include <lib/container_of.h>
#include <lib/list.h>
#include <scheduler.h>
#include <stdlib.h>
#include <utils.h>

list2 *sleep_queue_head = NULL;

void print_sleep_queue()
{
    list2 *pos = NULL;

    list2_for_each(pos, sleep_queue_head)
    {
        struct sleep_queue_entry *entry =
            LIST_ENTRY(pos, struct sleep_queue_entry, l);

        printf("%d ", entry->ticks);
    }

    printf("\n");
}

void add_sleep_queue(struct task *task, size_t time)
{
    irq_state_t irq_state = irq_save_disable();

    struct sleep_queue_entry *new = malloc(sizeof(struct sleep_queue_entry));

    size_t ticks = time;

    list2 *pos = NULL;
    list2 *prev = NULL;
    list2_for_each(pos, sleep_queue_head)
    {
        struct sleep_queue_entry *entry =
            LIST_ENTRY(pos, struct sleep_queue_entry, l);

        /* printf("new value:%d, current:%d, item:%d\n", ticks - entry->ticks,
         */
        /*        ticks, entry->ticks); */

        if ((int)(ticks - entry->ticks) < 0)
        {
            break;
        }

        prev = pos;
        ticks -= entry->ticks;
    }

    /* printf("adding with:%d time, %d ticks\n", time, ticks); */

    new->task = task;
    new->ticks = ticks;

    if (sleep_queue_head == NULL)
    {
        sleep_queue_head = &new->l;
    }
    else if (prev == NULL)
    {
        list2_add_front(&new->l, &sleep_queue_head);
    }
    else
    {
        list2_insert_after(&new->l, prev);
    }
    /* print_sleep_queue(); */

    remove_task(task);

    irq_restore(irq_state);
    schedule();
}

void increment_sleep_queue()
{

    if (sleep_queue_head == NULL)
    {
        return;
    }

    struct sleep_queue_entry *head =
        LIST_ENTRY(sleep_queue_head, struct sleep_queue_entry, l);

    if (head->task == NULL)
    {
        return;
    }

    if ((int)head->ticks > 0)
    {
        /* printf("ticks:%d\n", head->ticks); */
    }

    if (head->ticks == 0)
    {

        /* printf("head:%p\n", sleep_queue_head); */
        /* printf("next:%p\n", sleep_queue_head->next); */

        /* MIGHT NOT WORK */
        list2_remove(sleep_queue_head, &sleep_queue_head);
        add_task(head->task);

        /* printf("after:%p\n", sleep_queue_head); */
        /* if (sleep_queue_head != NULL) */
        /* { */
        /*     printf("next:%p\n\n", sleep_queue_head->next); */
        /* } */

        free(head);
    }
    head->ticks--;
}
