#ifndef LIBC_UTILS
#define LIBC_UTILS
#include <stdint.h>
#include <types.h>

#if defined(__is_libc)
extern int syscall_wrapper_asm(ptr_t ax, ptr_t bx, ptr_t cx, ptr_t dx, ptr_t si,
                               ptr_t di);
#endif

#endif
