#ifndef SYSCALL_KERNEL_H
#define SYSCALL_KERNEL_H

#include <kernel/types.h>

typedef ptr (*syscall_func_t)(ptr num, ptr, ptr, ptr, ptr, ptr);

extern syscall_func_t syscall_table[];

void syscall_add(ptr num, syscall_func_t);
ptr syscall(ptr num, ptr, ptr, ptr, ptr, ptr);

#define SYSCALL_DEFINE_GENERIC(func, num, body)                                \
    void __sys_##func(ptr a1, ptr a2, ptr a3, ptr a4, ptr a5){body}            \
                                                                               \
    __constructor void __sys_##func_init()                                     \
    {                                                                          \
        /* TODO: dont cast to void* ? */                                       \
        syscall_add(num, (void *)func);                                        \
    }

#define SYSCALL_DEFINE0(func, num)                                             \
    SYSCALL_DEFINE_GENERIC(func, num, syscall(num, 0, 0, 0, 0, 0););

#define SYSCALL_DEFINE1(func, num)                                             \
    SYSCALL_DEFINE_GENERIC(func, num, syscall(num, a1, 0, 0, 0, 0););

#endif
