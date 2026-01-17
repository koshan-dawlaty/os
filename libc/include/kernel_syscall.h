#ifndef KERNEL_SYSCALL_H
#define KERNEL_SYSCALL_H

#ifdef __is_libk
#include <kernel/syscall.h>
#else
#define SYSCALL_DEFINE0(num, ret, ...)                                         \
    ret __sys_fake_libc_##num##_test123(__VA_ARGS__)
#define SYSCALL_DEFINE1(num, ret, ...)                                         \
    ret __sys_fake_libc_##num##_test123(__VA_ARGS__)
#define SYSCALL_DEFINE2(num, ret, ...)                                         \
    ret __sys_fake_libc_##num##_test123(__VA_ARGS__)
#define SYSCALL_DEFINE3(num, ret, ...)                                         \
    ret __sys_fake_libc_##num##_test123(__VA_ARGS__)
#define SYSCALL_DEFINE4(num, ret, ...)                                         \
    ret __sys_fake_libc_##num##_test123(__VA_ARGS__)
#define SYSCALL_DEFINE5(num, ret, ...)                                         \
    ret __sys_fake_libc_##num##_test123(__VA_ARGS__)
#endif

#endif
