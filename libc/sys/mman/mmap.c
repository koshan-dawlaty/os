#include <lib/log.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <syscall.h>
#include <utils.h>

#ifdef __is_libk
#include <kernel/addr_space.h>
#include <kernel/paging.h>
#include <memory/paging_flags.h>
#include <scheduler.h>
#endif

#include <kernel_syscall.h>

void *mmap(void *addr, size_t len, int prot, int flags, off_t off)
{
#ifdef __is_libk
    addr_space_map_current((void *)addr, (len / 4096) + 1,
                           PAGING_USER | PAGING_WRITEABLE);
    return addr;
#else
    syscall_wrapper_asm(SYS_mmap, (uintptr_t)addr, len, prot, flags, 0);

    return addr;
#endif
}

SYSCALL_DEFINE5(SYS_mmap, ptr_t, void *, addr, size_t, len, int, prot, int,
                flags, off_t, off)
{
    return (ptr_t)mmap(addr, len, prot, flags, off);
}
