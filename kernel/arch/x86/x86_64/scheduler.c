#include "scheduler.h"

/*
 * prepares a new usermode task to jump to the usermode task initaliziation
 * function
 */
void scheduler_set_regs_arguments(int argc, char **argv, ptr_t ip,
                                  struct registers *regs)
{
    /* regs->edi = argc; */
    /* regs->esi = (ptr_t)argv; */
    /* regs->edx = (ptr_t)ip; */
}

void scheduler_set_regs_argv(int argc, char **argv, struct registers2 *regs)
{
    regs->edi = argc;
    regs->esi = (u64)argv;
}

void scheduler_set_first_arg(void *first_arg, struct registers *regs)
{
    /* regs->edi = (ptr_t)first_arg; */
}
