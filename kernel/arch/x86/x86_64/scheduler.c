#include "scheduler.h"

/*
 * prepares a new usermode task to jump to the usermode task initaliziation
 * function
 */
void scheduler_set_regs_arguments(int argc, char **argv, ptr ip,
                                  struct registers *regs)
{
    regs->edi = argc;
    regs->esi = (ptr)argv;
    regs->edx = (ptr)ip;
}

void scheduler_set_regs_argv(int argc, char **argv, struct registers *regs)
{
    regs->edi = argc;
    regs->esi = (u64)argv;
}

void scheduler_set_first_arg(void *first_arg, struct registers *regs)
{
    regs->edi = (ptr)first_arg;
}
