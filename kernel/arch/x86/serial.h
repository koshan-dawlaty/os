#ifndef SERIAL_H
#define SERIAL_H

#include "interrupts/idt.h"
#include <stdint.h>

void serial_write(uint16_t port, uint8_t data);
void serial_init(uint16_t port);
ISR_ATTRIBUTE void com1_interrupt_handler(struct interrupt_frame *frame);

#endif
