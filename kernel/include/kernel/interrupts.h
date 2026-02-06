#ifndef INTERRUPTS_H
#define INTERRUPTS_H
/* TODO: make architecture specific macro definitions for numbers, like INT_SYSCALL is 0x80 on x86 */

/*
 * makes @func accessible at interrupt number @num
 *
 * the arguments that @func should take may vary depending on the architecture
 *
 * this function should be called with interrupts off
 */
void register_interrupt_handler(int num, void * func);

#endif