#ifndef SYSCALL_KERNEL_H
#define SYSCALL_KERNEL_H

#include <compiler.h>
#include <kernel/types.h>

typedef ptr (*syscall_func_t)(ptr, ptr, ptr, ptr, ptr);

extern syscall_func_t syscall_table[];

void syscall_add(ptr num, syscall_func_t);
ptr kernel_syscall(ptr num, ptr, ptr, ptr, ptr, ptr);

#define SYSCALL_DEFINE_GENERIC(func, num, body, ...)                                \
    void __sys_##func(ptr a1, ptr a2, ptr a3, ptr a4, ptr a5){{__VA_ARGS__};{body};}            \
                                                                               \
    __constructor void __sys_##func##_init()                                   \
    {                                                                          \
        /* TODO: dont cast to void* ? */                                       \
        syscall_add(num, (void *)func);                                        \
    }

#define SYSCALL_DEFINE0(func, num, ...)                                             \
    SYSCALL_DEFINE_GENERIC(func, num, kernel_syscall(num, 0, 0, 0, 0, 0);, __VA_ARGS__);

#define SYSCALL_DEFINE1(func, num, ...)                                             \
    SYSCALL_DEFINE_GENERIC(func, num, kernel_syscall(num, a1, 0, 0, 0, 0);, __VA_ARGS__ );

#define SYSCALL_DEFINE2(func, num,...)                                             \
    SYSCALL_DEFINE_GENERIC(func, num, kernel_syscall(num, a1, a2, 0, 0, 0);, __VA_ARGS__ );

#define SYSCALL_DEFINE3(func, num, ...)                                             \
    SYSCALL_DEFINE_GENERIC(func, num, kernel_syscall(num, a1, a2, a3, 0, 0);, __VA_ARGS__ );

#define SYSCALL_DEFINE4(func, num, ...)                                             \
    SYSCALL_DEFINE_GENERIC(func, num, kernel_syscall(num, a1, a2, a3, a4, 0);, __VA_ARGS__ );

#define SYSCALL_DEFINE5(func, num, ...)                                             \
    SYSCALL_DEFINE_GENERIC(func, num, kernel_syscall(num, a1, a2, a3, a4, a5);, __VA_ARGS__ );

#endif
