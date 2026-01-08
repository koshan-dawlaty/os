#include <errno.h>
#include <kernel/syscall.h>
#include <kernel/types.h>
#include <stdlib.h>

#define MAX_SYSCALLS 256
syscall_func_t syscall_table[MAX_SYSCALLS];

void syscall_add(ptr num, syscall_func_t f)
{
    syscall_table[num] = f;
}

ptr kernel_syscall(ptr num, ptr b, ptr c, ptr d, ptr si, ptr di)
{
    if (syscall_table[num] == NULL)
    {
        return -ENOSYS;
    }
    return syscall_table[num](b, c, d, si, di);
}
