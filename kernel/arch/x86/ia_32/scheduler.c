#include "scheduler.h"

/*
 * prepares a new usermode task to jump to the usermode task initaliziation
 * function
 */
void scheduler_set_regs_arguments(int argc, char **argv, ptr ip,
                                  struct registers *regs)
{
    ptr *stack = (void *)regs->esp;
    stack[2] = argc;
    stack[3] = (ptr)argv;
    stack[4] = ip;
}

void scheduler_set_regs_argv(int argc, char **argv, struct registers *regs)
{
    ptr *stack = (void *)regs->esp;
    stack[0] = argc;
    stack[1] = (ptr)argv;
}
