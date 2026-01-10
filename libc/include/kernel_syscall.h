#ifndef KERNEL_SYSCALL_H
#define KERNEL_SYSCALL_H

#ifdef __is_libk
#include <kernel/syscall.h>
#else
#define SYSCALL_DEFINE0(...)
#define SYSCALL_DEFINE1(...)
#define SYSCALL_DEFINE2(...)
#define SYSCALL_DEFINE3(...)
#define SYSCALL_DEFINE4(...)
#define SYSCALL_DEFINE5(...)
#endif

#endif
