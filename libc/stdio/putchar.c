#include <stdio.h>
#include <utils.h>

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
