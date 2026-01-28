#ifndef USERSPACE_MOUSE_H
#define USERSPACE_MOUSE_H

#include <compiler.h>
#include <lib/list.h>
#include <types.h>

struct mouse_event
{
    int x_change;
    int y_change;

} __packed;

#endif
