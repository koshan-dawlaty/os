#ifndef KERNEL_SYSCALL_H
#define KERNEL_SYSCALL_H

#ifdef __is_libk
#include <kernel/syscall.h>
#else
#define SYSCALL_DEFINE0(num, ret) ret __sys_fake_libc_##num##_test123()
#define SYSCALL_DEFINE1(num, ret, t1, a1)                                      \
    ret __sys_fake_libc_##num##_test123(t1 a1)
#define SYSCALL_DEFINE2(num, ret, t1, a1, t2, a2)                              \
    ret __sys_fake_libc_##num##_test123(t1 a1, t2 a2)
#define SYSCALL_DEFINE3(num, ret, t1, a1, t2, a2, t3, a3)                      \
    ret __sys_fake_libc_##num##_test123(t1 a1, t2 a2, t3 a3)
#define SYSCALL_DEFINE4(num, ret, t1, a1, t2, a2, t3, a3, t4, a4)              \
    ret __sys_fake_libc_##num##_test123(t1 a1, t2 a2, t3 a3, t4 a4)
#define SYSCALL_DEFINE5(num, ret, t1, a1, t2, a2, t3, a3, t4, a4, t5, a5)      \
    ret __sys_fake_libc_##num##_test123(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5)
#endif

#endif
