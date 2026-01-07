#include <kernel/types.h>

typedef ptr (*syscall_func_t)(ptr, ptr, ptr, ptr, ptr, ptr);


#define MAX_SYSCALLS 256
syscall_func_t syscall_table[MAX_SYSCALLS];

void syscall_add(ptr num, syscall_func_t f)  
{
    syscall_table[num] = f;
}

ptr syscall(ptr num, ptr b, ptr c, ptr d,   ptr si, ptr di) {
    return syscall_table[num](num, b, c, d, si, di);
}