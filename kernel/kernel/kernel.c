#include "ata/ata.h"
#include "kernel/keyboard.h"
#include "kernel/paging.h"
#include "pci/pci.h"
#include "pit/pit.h"
#include <kernel/tty.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <vfs.h>

// TODO: bad
#include "../fs/ext2/ext2.h"

void kernel_init()
{
    terminal_initialize();
    init_paging();
}

superblock *init_ext2()
{

    ext2_fs_type.mount();

    // bad
    superblock *sb = ext2_fs_type.superblock_functions.parse_superblock();
    return sb;
}

int main()
{
    // useless
    keyboard_init();

    init_pit();
    init_pci();

    init_ata();

    superblock *ext2_sb = init_ext2();

    printf("ext2_sb:%p\n", ext2_sb);

    printf("type:%p\n", ext2_sb->type);
    printf("function:%p\n", ext2_sb->type->dentry_functions.find_dentry);

    printf("calling find\n");
    dentry *entry =
        ext2_sb->type->dentry_functions.find_dentry("/test.txt", ext2_sb);
    // printf("size:%p\n", entry);

    printf("%s\n", entry->name);

    uint8_t *buf = malloc(100);

    printf("\n");
    printf("%p\n", entry->inode);

    ext2_sb->type->inode_functions.read_inode(entry->inode, buf, 4, 0, ext2_sb);

    for (int i = 0; i < 4; i++)
    {
        printf("%c", buf[i]);
    }

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
