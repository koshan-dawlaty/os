#ifndef INTERRUPTS_H
#define INTERRUPTS_H
#include <types.h>
#include <interrutpts/arch_interrupts.h>

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
