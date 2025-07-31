#ifndef LIBC_UTILS
#define LIBC_UTILS
#include <stdint.h>

#if defined(__is_libc)
extern int syscall_wrapper_asm(uint32_t eax, uint32_t ebx, uint32_t ecx,
                               uint32_t edx, uint32_t esi, uint32_t edi);
#endif

#endif
