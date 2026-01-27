#ifndef USERSPACE_MOUSE_H
#define USERSPACE_MOUSE_H

#include <compiler.h>
#include <lib/list.h>
#include <types.h>

struct mouse_event
{
    u8 x_change;
    u8 y_change;

} __packed;

#endif
