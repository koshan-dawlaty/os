#include "stdio.h"
#include <kernel/keyboard.h>

char getchar()
{

    /* printf("test\n"); */
    /* printf("%d\n", key_available); */
    while (!key_available)
    {
        /* printf("%d\n", key_available); */
        /* printf("test\n"); */
        asm volatile("hlt");
    }

    key_available = 0;
    char c = pop_keyboard_buffer();
    return c;
}
