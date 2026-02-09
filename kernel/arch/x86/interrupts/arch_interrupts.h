#ifndef ARCH_INTERRUPTS_H
#define ARCH_INTERRUPTS_H
#include <interrupts/idt.h>

#define ARCH_INTERRUPT_HANDLER_DEFINE(name, body) ISR(name, body)
#define ARCH_INTERRUPT_ATTRIBUTE ISR_ATTRIBUTE
#define INT_SYSCALL 0x80

#endif