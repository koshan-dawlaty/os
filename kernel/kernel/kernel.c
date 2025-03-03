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

#ifdef TESTS
    run_all_tests();
#endif

    // jump_usermode();
    init_ext2();

    // ata_drive *d = get_ata_drive(0);

    // uint8_t *buffer = malloc(1024);
    // int index = 0x105;
    // int lba = ((index) * 1024) / 512;
    // // printf("index:%p, lba:%p, byte:%p\n", index, lba, index * 1024);
    // read_drive(d, buffer, 1024, lba);

    // for (int i = 0; i < 512; i++)
    // {
    //     printf("%p ", buffer[i]);
    // }

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
