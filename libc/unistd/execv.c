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
    return syscall_wrapper_asm(2, (uint32_t)path, (uint32_t)argv, 0, 0, 0);

#else

    int f = open(path, 0);

    if (f < 0)
    {
        return -ENOENT;
    }

    elf_32 *elf = parse_elf32(f);
    if (elf != NULL)
    {

        int argc = 0;
        while (argv[argc] != NULL)
        {
            argc++;
        }

        add_task(load_elf32(elf, argc, argv));
    }
    else
    {
        return -ENOEXEC;
    }

    return 0;
#endif
}
