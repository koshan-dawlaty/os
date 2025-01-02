#include "kernel/keyboard.h"
#include "kernel/paging.h"
#include "memory/paging_flags.h"
#include <kernel/tty.h>
#include <stddef.h>
#include <stdio.h>

int main()
{
    terminal_initialize();
    init_paging();

    int *one = kmalloc(12);
    int *two = kmalloc(12);
    *one = 12345;
    printf("%d\n", *one);
    printf("%p\n", one);
    kfree(one);

    *two = 67890;
    printf("%d\n", *two);
    printf("%p\n", two);
    kfree(two);

    keyboard_reset();
    keyboard_set_default();
    keyboard_set_typematic(0, 0);
    keyboard_set_led(KEYBOARD_LED_CAPS_LOCK);

    while (1)
    {
        char c = getchar();
        printf("%c", c);
    }
    /* printf("test\n"); */

    return 0;
}
