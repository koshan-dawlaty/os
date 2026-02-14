#ifndef X86_KEYBOARD_H
#define X86_KEYBOARD_H

#include "../interrupts/idt.h"
#include "kernel/interrupts.h"

#define KEYBOARD_PORT_COMMAND 0x64
#define KEYBOARD_PORT_DATA 0x60

#define KEYBOARD_ACKNOWLEDGED 0xFA
#define KEYBOARD_RESEND 0xFE

void __int_handler keyboard_interrupt_handler(struct interrupt_frame *frame);

#endif
