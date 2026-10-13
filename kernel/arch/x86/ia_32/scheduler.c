#include "scheduler.h"

/*
 * prepares a new usermode task to jump to the usermode task initaliziation
 * function
 */
void scheduler_set_regs_arguments(int argc, char **argv, ptr_t ip,
                                  struct registers *regs)
{
}

void scheduler_set_regs_argv(int argc, char **argv, struct registers2 *regs)
{
    ptr_t *stack = (void *)(ptr_t)regs->esp;
    stack[0] = argc;
    stack[1] = (ptr_t)argv;
}

void scheduler_set_first_arg(void *first_arg, struct registers *regs)
{
}
