#include "ata/ata.h"
#include "fs/ext2.h"
#include "kernel/keyboard.h"
#include "kernel/paging.h"
#include "pci/pci.h"
#include "pit/pit.h"
#include "tests/tests.h"
#include "utils.h"
#include <kernel/tty.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

void kernel_init()
{
    terminal_initialize();
    init_paging();
}

int main()
{
    // // useless
    keyboard_init();

    init_pit();
    init_pci();

    init_ata();
    init_ext2();

#ifdef TESTS
    run_all_tests();
#endif

    // read_inode(node, buf, node_size, 0);

    // for (size_t i = 0; i < node_size; i++)
    // {
    //     printf("%d, %d\n", expected[i], buf[i]);
    // }

    // jump_usermode();

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
