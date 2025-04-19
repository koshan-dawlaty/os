#include "ata/ata.h"
#include "elf.h"
#include "kernel/keyboard.h"
#include "kernel/paging.h"
#include "kernel/usermode.h"
#include "pci/pci.h"
#include "pit/pit.h"
#include <fcntl.h>
#include <kernel/tty.h>
#include <memory/paging_flags.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

    init_vfs();

    printf("test\n");
    /* uint32_t val = 0x8048000; */

    /* map_page((void *)0xC424000, (void *)val, PAGING_WRITEABLE); */

    /* uint8_t *ptr = (uint8_t *)val; */
    /* ptr[0] = 5; */
    /* printf("%d\n", ptr[0]); */
    /* printf("%d\n", ptr[0]); */

    /* printf("before\n"); */
    /* printf("%p\n", val); */
    /* printf("%p\n", val); */
    /* printf("%p\n", val); */

    init_usermode();
    /* /1* jump_usermode(); *1/ */

    int f = open("/usr/bin/elf_test", 0);

    elf_32 *elf = parse_elf32(f);

    load_elf32(elf);
    /* jump_usermode_asm(); */
    /* printf("after\n"); */
    /* printf("%p\n", val); */
    /* *printf("%p\n", val); */
    /* printf("%p\n", val); */
    // printf("%d\n", elf->program_headers_len);

    // char *buf = malloc(100);

    // pread(f, buf, 4, 3);

    // printf("%s\n", buf);

    // printf("read %d bytes\n", read(f, buf, 10));
    // printf("read %d bytes\n", read(f, buf + 10, 3));
    // printf("read %d bytes\n", read(f, buf + 13, 5));
    // printf("%s\n", buf);

    // file *f = open_file("/test.txt");

    // char *buf = malloc(100);

    // printf("read %d bytes\n", read_file(f, buf, 100, 7));

    // printf("%s\n", buf);

    // superblock *ext2_sb = init_ext2();

    // printf("ext2_sb:%p\n", ext2_sb);

    // printf("type:%p\n", ext2_sb->type);
    // printf("function:%p\n", ext2_sb->type->dentry_functions.find_dentry);

    // printf("calling find\n");
    // dentry *entry =
    //     ext2_sb->type->dentry_functions.find_dentry("/test.txt",
    //     ext2_sb);
    // // printf("size:%p\n", entry);

    // printf("%s\n", entry->name);

    // uint8_t *buf = malloc(100);

    // printf("\n");
    // printf("size:%p\n", entry->inode->size);

    // ext2_sb->type->inode_functions.read_inode(entry->inode, buf,
    //                                           entry->inode->size, 0,
    //                                           ext2_sb);

    // printf("%s\n", buf);

    // file *f = open_file("/test.txt");

#ifdef TESTS
    run_all_tests();
#endif

    // read_inode(node, buf, node_size, 0);

    // for (size_t i = 0; i < node_size; i++)
    // {
    //     printf("%d, %d\n", expected[i], buf[i]);
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
