#include <stdio.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <syscall.h>
#include <utils.h>

#ifdef __is_libk
#include <kernel/addr_space.h>
#include <kernel/paging.h>
#include <memory/paging_flags.h>
#endif

#include <kernel_syscall.h>

void *mmap(void *addr, size_t len, int prot, int flags, off_t off)
{
#ifdef __is_libk
    for (size_t i = 0; i < (len / 4096) + 1; i++)
    {
        /* TODO: make a function to map to the current addr_space because this
         * is stupid and will probably break if current_addr_space is not
         * updated correctly */
        addr_space_map(&current_addr_space, (void *)addr + (i * 4096), 1,
                       PAGING_USER | PAGING_WRITEABLE);
    }
    return addr;
#else
    syscall_wrapper_asm(SYS_mmap, (uintptr_t)addr, len, 0, 0, 0);

    return addr;
#endif
}

SYSCALL_DEFINE5(SYS_mmap, ptr, void *, addr, size_t, len, int, prot, int, flags,
                off_t, off)
{
    return (ptr)mmap(addr, len, prot, flags, off);
}
