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
    printf("char:%c\n", c);

    return c;

#else
    char c;
    syscall_wrapper_asm(1, &c, 0, 0, 0, 0);
    return c;

#endif
}
