#ifndef SYSCALL_KERNEL_H
#define SYSCALL_KERNEL_H

#include <kernel/types.h>

typedef ptr (*syscall_func_t)(ptr num, ptr, ptr, ptr, ptr, ptr);

extern syscall_func_t syscall_table[];

void syscall_add(ptr num, syscall_func_t);
ptr syscall(ptr num, ptr, ptr, ptr, ptr, ptr);

#endif