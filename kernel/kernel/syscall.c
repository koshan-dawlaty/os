#include <errno.h>
#include <kernel/syscall.h>
#include <lib/assert.h>
#include <stdlib.h>
#include <types.h>

#define MAX_SYSCALLS 256
syscall_func_t syscall_table[MAX_SYSCALLS];

void syscall_add(ptr_t num, syscall_func_t f)
{
    ASSERT(num < MAX_SYSCALLS, "syscall added at invalid index");
    syscall_table[num] = f;
}

ptr_t kernel_syscall(ptr_t num, ptr_t b, ptr_t c, ptr_t d, ptr_t si, ptr_t di)
{
    if (syscall_table[num] == NULL)
    {
        return -ENOSYS;
    }
    return syscall_table[num](b, c, d, si, di);
}
