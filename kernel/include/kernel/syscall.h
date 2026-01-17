#ifndef SYSCALL_KERNEL_H
#define SYSCALL_KERNEL_H

#include <compiler.h>
#include <kernel/types.h>

typedef ptr (*syscall_func_t)(ptr, ptr, ptr, ptr, ptr);

extern syscall_func_t syscall_table[];

void syscall_add(ptr num, syscall_func_t);
ptr kernel_syscall(ptr num, ptr, ptr, ptr, ptr, ptr);

#define SYSCALL_INTERNAL_FUNC(num) __sys_##num##_func

#define SYSCALL_DEFINE_GENERIC(num, return_type, body, ...)                    \
    return_type __sys_##num##_func(__VA_ARGS__);                               \
                                                                               \
    ptr __sys_##num##_func_real(ptr a1, ptr a2, ptr a3, ptr a4, ptr a5)        \
    {                                                                          \
        {                                                                      \
            body                                                               \
        }                                                                      \
    }                                                                          \
    __constructor void __sys_##num##_init()                                    \
    {                                                                          \
        /* TODO: dont cast to void * */                                        \
        syscall_add(num, __sys_##num##_func_real);                             \
    }                                                                          \
    return_type __sys_##num##_func(__VA_ARGS__)

#define __MACRO_ARG_CAST(t, x) ((t)(x))

#define SYSCALL_DEFINE0(num, return_type)                                      \
    SYSCALL_DEFINE_GENERIC(num, return_type, SYSCALL_INTERNAL_FUNC(num)();)

#define SYSCALL_DEFINE1(num, return_type, t1, a1)                              \
    SYSCALL_DEFINE_GENERIC(num, return_type, SYSCALL_INTERNAL_FUNC(num)(a1);   \
                           , __VA_ARGS__)
#define SYSCALL_DEFINE2(num, return_type, ...)                                 \
    SYSCALL_DEFINE_GENERIC(                                                    \
        num, return_type, SYSCALL_INTERNAL_FUNC(num)(a1, a2);, __VA_ARGS__)

#define SYSCALL_DEFINE3(num, return_type, ...)                                 \
    SYSCALL_DEFINE_GENERIC(num, return_type,                                   \
                           SYSCALL_INTERNAL_FUNC(num)(a1, a2, a3);             \
                           , __VA_ARGS__)

#define SYSCALL_DEFINE4(num, return_type, ...)                                 \
    SYSCALL_DEFINE_GENERIC(num, return_type,                                   \
                           SYSCALL_INTERNAL_FUNC(num)(a1, a2, a3, a4);         \
                           , __VA_ARGS__)

#define SYSCALL_DEFINE5(num, return_type, ...)                                 \
    SYSCALL_DEFINE_GENERIC(num, return_type,                                   \
                           SYSCALL_INTERNAL_FUNC(num)(a1, a2, a3, a4, a5);     \
                           , __VA_ARGS__)

#endif
