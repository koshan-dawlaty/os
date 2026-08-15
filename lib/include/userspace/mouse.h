#ifndef USERSPACE_MOUSE_H
#define USERSPACE_MOUSE_H

#include <compiler.h>
#include <lib/list.h>
#include <types.h>

struct mouse_buttons
{
    /* TODO: (maybe) it would be very nice if bitfields could be used here, but
     * it seems like the order is not guaranteed, even for a packed struct */
    bool left_click;
    bool right_click;
    bool middle_click;
} __packed;

#define MOUSE_BUTTONS_EMPTY(b)                                                 \
    (!b.left_click && !b.right_click && !b.middle_click)

struct mouse_event
{
    int x_change;
    int y_change;
    struct mouse_buttons buttons;
    int scroll;

} __packed;

#define MOUSE_EVENT_EMPTY(ev)                                                  \
    (ev.x_change == 0 && ev.y_change == 0 && MOUSE_BUTTONS_EMPTY(ev.buttons))
void mouse_ps2_interrupt();

#endif
