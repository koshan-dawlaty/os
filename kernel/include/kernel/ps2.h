#ifndef PS2_H
#define PS2_H

#include <interrupts/idt.h>
#include <kernel/interrupts.h>
#include <kernel/types.h>

int ps2_init();

/* returns -1 on error, 0 on success */
int ps2_write_dev(u8 byte, bool is_port2);
u8 ps2_read_poll(bool is_port2);
void __int_handler ps2_int_handler_1(struct interrupt_frame *frame);
void __int_handler ps2_int_handler_12(struct interrupt_frame *frame);

#endif
