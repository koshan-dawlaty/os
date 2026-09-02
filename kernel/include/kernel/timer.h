#ifndef TIMER_H
#define TIMER_H

#include <lib/list.h>
#include <stddef.h>

struct timer_callback_data
{
};

typedef void (*timer_callback_t)(struct timer_callback_data *data);

struct timer_event
{
    struct timer_callback_data *data;
    timer_callback_t callback;
    size_t time_diff;
    struct list list;
};

struct timer_event *timer_event_alloc(timer_callback_t callback,
                                      struct timer_callback_data *data);
/*
 * called by hardware timer interrupt handler
 */
void timer_events_update();
void timer_event_add(struct timer_event *ev, size_t time);

#endif
