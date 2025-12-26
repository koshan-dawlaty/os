#include "utils.h"
#include <stdint.h>
#include <sys/mman.h>
#include <syscall.h>

void *mmap(void *addr, size_t len, int prot, int flags, int fildes, off_t off)
{
    syscall_wrapper_asm(SYS_mmap, (uintptr_t)addr, len, 0, 0, 0);

    return addr;
}
