#include <stdio.h>

#if defined(__is_libc)
extern void syscall_wrapper_asm(uint32_t eax, uint32_t ebx, uint32_t ecx,
                                uint32_t edx, uint32_t esi, uint32_t edi);
#endif

#if defined(__is_libk)
#include <kernel/tty.h>
#endif

int putchar(int ic)
{
#if defined(__is_libk)
    char c = (char)ic;
    terminal_write(&c, sizeof(c));
#else
    syscall_wrapper_asm(0, ic, 0, 0, 0, 0);
#endif
    return ic;
}
