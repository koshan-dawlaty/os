#include "kernel/keyboard.h"
#include "kernel/paging.h"
#include "keyboard/x86_keyboard.h"
#include "tests/tests.h"
#include <kernel/tty.h>
#include <stddef.h>
#include <stdio.h>
#define TEST_DEBUG

int main()
{
    terminal_initialize();
    init_paging();
    // useless
    keyboard_init();

#ifndef NO_TESTS
    run_all_tests();
#endif

    while (1)
    {
        char c = getchar();
        printf("%c", c);
    }

    return 0;
}
