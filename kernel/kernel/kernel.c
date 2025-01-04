#include "kernel/keyboard.h"
#include "kernel/paging.h"
#include "tests/tests.h"
#include <kernel/tty.h>
#include <stddef.h>
#include <stdio.h>
#define TEST_DEBUG

int main()
{
    terminal_initialize();
    init_paging();

#ifndef NO_TESTS
    run_all_tests();
#endif

    /* keyboard_reset(); */
    /* keyboard_set_default(); */
    /* keyboard_set_typematic(0, 0); */
    /* keyboard_set_led(KEYBOARD_LED_CAPS_LOCK); */

    while (1)
    {
        char c = getchar();
        printf("%c", c);
    }

    return 0;
}
