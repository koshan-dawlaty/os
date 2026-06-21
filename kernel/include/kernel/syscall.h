#ifndef SYSCALL_KERNEL_H
#define SYSCALL_KERNEL_H

#include <compiler.h>
#include <types.h>

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
        syscall_add(num, __sys_##num##_func_real);                             \
    }                                                                          \
    return_type __sys_##num##_func(__VA_ARGS__)

#define __MACRO_ARG_CAST(t, x) ((t)(x))

#define SYSCALL_DEFINE0(num, return_type)                                      \
    SYSCALL_DEFINE_GENERIC(num, return_type,                                   \
                           return SYSCALL_INTERNAL_FUNC(num)();)

#define SYSCALL_DEFINE1(num, return_type, t1, arg1)                            \
    SYSCALL_DEFINE_GENERIC(                                                    \
        num, return_type,                                                      \
        return SYSCALL_INTERNAL_FUNC(num)(__MACRO_ARG_CAST(t1, a1));           \
        , t1 arg1)

#define SYSCALL_DEFINE2(num, return_type, t1, arg1, t2, arg2)                  \
    SYSCALL_DEFINE_GENERIC(                                                    \
        num, return_type,                                                      \
        return SYSCALL_INTERNAL_FUNC(num)(__MACRO_ARG_CAST(t1, a1),            \
                                          __MACRO_ARG_CAST(t2, a2));           \
        , t1 arg1, t2 arg2)

#define SYSCALL_DEFINE3(num, return_type, t1, arg1, t2, arg2, t3, arg3)        \
    SYSCALL_DEFINE_GENERIC(                                                    \
        num, return_type,                                                      \
        return SYSCALL_INTERNAL_FUNC(num)(__MACRO_ARG_CAST(t1, a1),            \
                                          __MACRO_ARG_CAST(t2, a2),            \
                                          __MACRO_ARG_CAST(t3, a3));           \
        , t1 arg1, t2 arg2, t3 arg3)

#define SYSCALL_DEFINE4(num, return_type, t1, arg1, t2, arg2, t3, arg3, t4,    \
                        arg4)                                                  \
    SYSCALL_DEFINE_GENERIC(                                                    \
        num, return_type,                                                      \
        return SYSCALL_INTERNAL_FUNC(num)(                                     \
            __MACRO_ARG_CAST(t1, a1), __MACRO_ARG_CAST(t2, a2),                \
            __MACRO_ARG_CAST(t3, a3), __MACRO_ARG_CAST(t4, a4));               \
        , t1 arg1, t2 arg2, t3 arg3, t4 arg4)

#define SYSCALL_DEFINE5(num, return_type, t1, arg1, t2, arg2, t3, arg3, t4,    \
                        arg4, t5, arg5)                                        \
    SYSCALL_DEFINE_GENERIC(                                                    \
        num, return_type,                                                      \
        return SYSCALL_INTERNAL_FUNC(num)(                                     \
            __MACRO_ARG_CAST(t1, a1), __MACRO_ARG_CAST(t2, a2),                \
            __MACRO_ARG_CAST(t3, a3), __MACRO_ARG_CAST(t4, a4),                \
            __MACRO_ARG_CAST(t5, a5));                                         \
        , t1 arg1, t2 arg2, t3 arg3, t4 arg4, t5 arg5)

#endif
