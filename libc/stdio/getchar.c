#include "stdio.h"
#include "utils.h"
#include <kernel/keyboard.h>
#include <unistd.h>

char getchar()
{
#if defined(__is_libk)

    /* printf("test\n"); */
    /* printf("%d\n", key_available); */
    while (!is_key_available())
    {
        /* printf("%d\n", key_available); */
        /* printf("test\n"); */
        asm volatile("hlt");
        /* sched_yield(); */
    }
    /* printf("test\n"); */

    key_available = 0;
    char c = pop_keyboard_buffer();

    return c;

#else
    char c = syscall_wrapper_asm(1, 0, 0, 0, 0, 0);

    /* TODO: this will make a lot of interrupts */
    while (c == 0)
    {
        c = syscall_wrapper_asm(1, 0, 0, 0, 0, 0);
    }
    return c;

#endif
}
