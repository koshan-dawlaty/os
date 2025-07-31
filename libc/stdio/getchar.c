#include "stdio.h"
#include <kernel/keyboard.h>

char getchar()
{
#if defined(__is_libk)

    /* printf("test\n"); */
    /* printf("%d\n", key_available); */
    while (!key_available)
    {
        /* printf("%d\n", key_available); */
        /* printf("test\n"); */
        asm volatile("hlt");
    }
    /* printf("test\n"); */

    key_available = 0;
    char c = pop_keyboard_buffer();

    return c;

#else
    char c = syscall_wrapper_asm(1, 0, 0, 0, 0, 0);

    if (c != 0)
    {
        /* printf("in getchar:%c\n", c); */
    }
    return c;

#endif
}
