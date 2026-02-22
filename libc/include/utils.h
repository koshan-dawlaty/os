#ifndef LIBC_UTILS
#define LIBC_UTILS
#include <stdint.h>
#include <types.h>

#if defined(__is_libc)
extern int syscall_wrapper_asm(ptr ax, ptr bx, ptr cx, ptr dx, ptr si, ptr di);
#endif

#endif
