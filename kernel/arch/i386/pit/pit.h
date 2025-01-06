#ifndef PIT_H
#define PIT_H

#include "interrupts/idt.h"
#include <stdint.h>

ISR_ATTRIBUTE void pit_handler(struct interrupt_frame *frame);

void init_pit();
uint64_t get_pit_time();
uint64_t get_seconds();

#endif
