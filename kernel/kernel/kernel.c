#include "ata/ata.h"
#include "kernel/keyboard.h"
#include "kernel/paging.h"
#include "pit/pit.h"
#include "tests/tests.h"
#include <kernel/tty.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

void kernel_init()
{
    terminal_initialize();
    init_paging();
}

int main()
{
    // useless
    keyboard_init();
    init_pit();
    init_ata();

#ifdef TESTS
    run_all_tests();
#endif

    /* jump_usermode(); */

    while (1)
    {
        char c = getchar();
        printf("%c", c);
    }

    return 0;
}
