#include "scheduler.h"

void scheduler_set_regs_arguments(int argc, char **argv, struct registers *regs)
{
    regs->esi = (ptr)argv;
    regs->edi = (ptr)argc;
}
