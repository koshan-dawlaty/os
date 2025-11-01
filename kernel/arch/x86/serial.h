#ifndef SERIAL_H
#define SERIAL_H

#include "interrupts/idt.h"
#include "kernel/types.h"
#include <stddef.h>
#include <stdint.h>

void serial_write(uint16_t port, uint8_t data);
void serial_init(uint16_t port);
void serial_writestring(char *str, size_t len, u16 port);
ISR_ATTRIBUTE void com1_interrupt_handler(struct interrupt_frame *frame);

#endif
