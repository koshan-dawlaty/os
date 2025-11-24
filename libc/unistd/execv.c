#include "fcntl.h"
#include "utils.h"
#include <errno.h>
#include <stdint.h>
#include <unistd.h>

#ifdef __is_libk
#include <elf.h>
#include <scheduler.h>
#endif

int execv(const char *path, char *const argv[])
{
#ifndef __is_libk
    return syscall_wrapper_asm(2, (uintptr_t)path, (uintptr_t)argv, 0, 0, 0);

#else

    int f = open((char *)path, 0);

    if (f < 0)
    {
        return -ENOENT;
    }

    elf *elf = parse_elf(f);
    if (elf != NULL)
    {

        int argc = 0;
        while (argv[argc] != NULL)
        {
            argc++;
        }

        add_task(load_elf(elf, argc, (char **)argv));
    }
    else
    {
        return -ENOEXEC;
    }

    close(f);

    return 0;
#endif
}
