#include <kernel/timer.h>
#include <lib/list.h>
#include <lib/log.h>
#include <stdlib.h>

/* TODO: this code could be used to replace scheduler/sleep.c, it implements the
 * same algorithim */

LIST_DEFINE_HEAD(timer_events);

struct timer_event *timer_event_alloc(timer_callback_t callback,
                                      struct timer_callback_data *data)
{
    struct timer_event *out = malloc(sizeof(struct timer_event));
    out->callback = callback;
    out->data = data;
    out->list = LIST_INIT();
    out->time_diff = 0;
    return out;
}

void timer_events_update()
{
    if (!LIST_IS_EMPTY(timer_events))
    {
        struct timer_event *ev =
            containerof(timer_events.next, struct timer_event, list);
        if (ev->time_diff <= 0)
        {
            list_pop(&timer_events);
            ev->callback(ev->data);
            free(ev);
        }
        else
        {
            ev->time_diff -= 1;
        }
    }
}

void timer_event_add(struct timer_event *new_ev, size_t time)
{
    size_t sum = 0;
    struct list *pos;
    list_for_each(pos, &timer_events)
    {
        if (time < sum)
        {
            break;
        }
        struct timer_event *ev = containerof(pos, struct timer_event, list);
        sum += ev->time_diff;
    }
    new_ev->time_diff = time - sum;
    list_add_after(&new_ev->list, pos);
}
