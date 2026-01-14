#include "utils.h"
#include <errno.h>
#include <fcntl.h>
#include <kernel_syscall.h>
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

    printf("execv start\n");
    /* TODO: wrong oflags */
    int f = open((char *)path, O_RDONLY);

    printf("file opened as:%d\n", f);
    if (f < 0)
    {
        return -ENOENT;
    }

    printf("starting to parse elf file\n");
    elf *elf = parse_elf(f);
    printf("elf file parsed\n");
    if (elf != NULL)
    {

        int argc = 0;
        /* printf("%p\n", argv[0]); */
        while (argv[argc] != NULL)
        {
            argc++;
        }
        printf("arguments parsed\n");
        struct task *t = load_elf(elf, argc, (char **)argv);
        printf("elf file loaded\n");
        add_task(t);
        printf("task added\n");
        free_elf(elf);
        printf("file freed\n");
        /* this the old task from running soon after this is called, it also
         * makes this function more responsive as the new task should be
         * immediately switched to
         */
        /* schedule(); */
    }
    else
    {
        return -ENOEXEC;
    }
    printf("execv end\n");

    close(f);

    return 0;
#endif
}

SYSCALL_DEFINE2(execv, SYS_execv);
