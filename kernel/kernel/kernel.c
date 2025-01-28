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

#ifdef TESTS
    run_all_tests();
#endif

    char *buffer = malloc(100);

    sprintf(buffer, "%xabc%sdef%s%d\n", 0x1234, "12345", "askjdfjasdf", 10);

    printf("%s\n", buffer);

    /* jump_usermode(); */

    while (1)
    {
        char c = getchar();

        if (c == 'k')
        {
            terminal_scroll_up();
            continue;
        }
        printf("%c", c);
    }

    return 0;
}
