#include "gdt.h"
#include "kernel/keyboard.h"
#include "kernel/paging.h"
#include "kernel/usermode.h"
#include "memory/paging_flags.h"
#include "pit/pit.h"
#include "tests/tests.h"
#include <kernel/tty.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main()
{
    terminal_initialize();
    init_paging();
    // useless
    keyboard_init();

    init_pit();

#ifdef TESTS
    run_all_tests();
#endif
    void *a = malloc(100);

    free(a);

    /* jump_usermode(); */

    /* while (1) */
    /* { */
    /*     char c = getchar(); */
    /*     printf("%c", c); */
    /* } */

    return 0;
}
