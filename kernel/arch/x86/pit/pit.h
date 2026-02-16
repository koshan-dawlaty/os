#ifndef PIT_H
#define PIT_H

#include "interrupts/idt.h"
#include "kernel/interrupts.h"
#include <stdint.h>

/* TODO: this may have wrong parameters, however it shouldnt matter as only the
 * idt should call this*/
extern void pit_handler_asm();

__int_handler void pit_handler(struct interrupt_frame *frame);

void init_pit();
uint64_t get_pit_time();
uint64_t get_seconds();

#endif
