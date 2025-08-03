#include "utils.h"
#include <sys/mman.h>

void *mmap(void *addr, size_t len, int prot, int flags, int fildes, off_t off)
{

    syscall_wrapper_asm(4, (uint32_t)addr, 0, 0, 0, 0);

    return (void *)(0x123456);
}
