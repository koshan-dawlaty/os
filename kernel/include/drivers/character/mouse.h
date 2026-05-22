#ifndef MOUSE_H
#define MOUSE_H
#include <compiler.h>
#include <interrupts/idt.h>
#include <kernel/interrupts.h>
#include <lib/list.h>
#include <userspace/mouse.h>

struct device *mouse_get();

#endif
