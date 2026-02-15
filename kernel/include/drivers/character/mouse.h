#ifndef MOUSE_H
#define MOUSE_H
#include <compiler.h>
#include <interrupts/idt.h>
#include <lib/list.h>
#include <userspace/mouse.h>

ISR_ATTRIBUTE void mouse_interrupt_handler(struct interrupt_frame *frame);
struct device *mouse_get();

#endif
