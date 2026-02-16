#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <interrupts/arch_interrupts.h>
#include <types.h>

#define __int_handler ARCH_INTERRUPT_ATTRIBUTE

/*
 * defines a function called name that is an interrupt handler.
 * to use the function in a header file use __int_handler when defining it.
 */
#define INT_HANDLER(name, body) ARCH_INTERRUPT_HANDLER_DEFINE(name, body)

typedef struct
{
    u8 is_user : 1;
} int_handler_flags;

/*
 * makes @func accessible at interrupt number @num
 *
 * the arguments that @func should take may vary depending on the architecture
 *
 * this function should be called with interrupts off
 */
void register_interrupt_handler(int num, void *func, int_handler_flags flags);

#endif
