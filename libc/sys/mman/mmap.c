#include "utils.h"
#include <stdint.h>
#include <sys/mman.h>

void *mmap(void *addr, size_t len, int prot, int flags, int fildes, off_t off)
{
    syscall_wrapper_asm(4, (uintptr_t)addr, 0, 0, 0, 0);

    return addr;
}
