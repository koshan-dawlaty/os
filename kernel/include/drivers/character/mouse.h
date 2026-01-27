#ifndef MOUSE_H
#define MOUSE_H
#include <compiler.h>
#include <interrupts/idt.h>
#include <lib/list.h>

struct mouse_event
{
    u8 x_change;
    u8 y_change;

    struct list2 list;
} __packed;

ISR_ATTRIBUTE void mouse_interrupt_handler(struct interrupt_frame *frame);
struct device *mouse_get();

#endif
