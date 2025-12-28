#include "utils.h"
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <syscall.h>
#include <unistd.h>

#ifdef __is_libk
#include <elf.h>
#include <scheduler.h>
#endif

int execv(const char *path, char *const argv[])
{
#ifndef __is_libk
    return syscall_wrapper_asm(SYS_execv, (uintptr_t)path, (uintptr_t)argv, 0,
                               0, 0);

#else

    /* TODO: wrong oflags */
    int f = open((char *)path, O_RDONLY);

    if (f < 0)
    {
        return -ENOENT;
    }

    elf *elf = parse_elf(f);
    if (elf != NULL)
    {

        int argc = 0;
        /* printf("%p\n", argv[0]); */
        while (argv[argc] != NULL)
        {
            argc++;
        }
        struct task *t = load_elf(elf, argc, (char **)argv);
        add_task(t);
        free_elf(elf);
        /* this the old task from running soon after this is called, it also
         * makes this function more responsive as the new task should be
         * immediately switched to
         */
        schedule();
    }
    else
    {
        return -ENOEXEC;
    }

    close(f);

    return 0;
#endif
}
