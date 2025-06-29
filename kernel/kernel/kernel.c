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
#include <vfs.h>

// TODO: bad
#include "../fs/ext2/ext2.h"
#include "scheduler.h"
#include "unistd.h"
#include "utils.h"

extern struct pagedir init_pgdir;

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

    init_usermode();

    /* struct pagedir *dir = new_pagedir(); */
    /* write_kernel_page_tables(dir); */
    /* load_pgdir(dir); */
    /* printf("%p\n", (dir->pgdir)); */
    /* printf("%p\n", (init_pgdir.pgdir)); */

    /* printf("%p\n", (dir->pgdir)); */
    /* printf("%p\n", (init_pgdir.pgdir)); */

    /* write_pagedir(dir, (void *)0x2000000, (void *)0x08048000, */
    /*               PAGING_WRITEABLE | PAGING_USER); */

    /* load_pgdir(&init_pgdir); */
    /* *(char *)(0x08048000) = 'a'; */

    /* printf("malloc:%p\n", malloc(1234)); */

    /* printf("12345 %p\n", dir->pgdir[1023]); */
    /* write_pagedir(dir, (void *)0x2000000, (void *)0x1000, PAGING_WRITEABLE);
     */
    /* write_pagedir(dir, (void *)0x2100000, (void *)0x804828c,
     * PAGING_WRITEABLE); */
    /* *(char *)0x804828c = 'a'; */

    /* printf("1234567:%p\n", ((uint32_t *)(virtual))[768]); */

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

    /* uint8_t *virtual = (void *)0x08048000; */

    /* struct pagedir *dir = new_pagedir(); */
    /* write_kernel_page_tables(dir); */

    /* load_pgdir(dir); */
    /* write_pagedir(dir, (void *)0x2000000, virtual, */
    /*               PAGING_WRITEABLE | PAGING_USER | PAGING_PRESENT); */

    /* int f = open("/usr/bin/elf_test", 0); */
    /* size_t size = 0x2e8; */
    /* size_t offset = 0; */
    /* uint8_t *buf = malloc(size); */
    /* pread(f, buf, size, offset); */
    /* virtual[610] = 'a'; */
    /* /1* virtual[611] = 'a'; *1/ */
    /* printf("%p\n", buf[size]); */

    /* for (size_t i = 0; i < 10; i++) */
    /* { */
    /*     /1* printf("%p ", buf[i]); *1/ */
    /* } */

    /* printf("%p\n", buf); */
    /* for (size_t i = 0; i < size; i++) */
    /* { */
    /*     printf("%d", i); */
    /*     virtual[i] = buf[i]; */
    /* } */

    /* for (size_t i = 0; i < size; i++) */
    /* { */
    /*     printf("%d", i); */
    /*     virtual[i] = buf[i]; */
    /* } */
    /* printf("done\n"); */

    int f1 = open("/usr/bin/elf_test", 0);
    elf_32 *elf = parse_elf32(f1);
    struct pagedir *dir1 = load_elf32(elf);

    /*
for (int i = 0; i < 1000000000; i++)
{
    io_wait();
}
    */

    int f2 = open("/usr/bin/elf_test2", 0);
    elf_32 *elf2 = parse_elf32(f2);
    struct pagedir *dir2 = load_elf32(elf2);

    /* int f3 = open("/usr/bin/elf_test", 0); */
    /* elf_32 *elf3 = parse_elf32(f3); */
    /* struct pagedir *dir3 = load_elf32(elf3); */

    enable_scheduler();
    load_pgdir(dir1);

    printf("test\n");

    jump_usermode_asm(elf->entry_point);

    /* printf("dir1:%p, dir2:%p\n", dir1, dir2); */

    /* load_pgdir(dir1); */

    /* jump_usermode_asm(elf->entry_point); */

    /* 0x6001AC */

    /* printf("entry: 1:%p, 2:%p\n", elf->entry_point, elf2->entry_point);
     */

    /* load_elf32(elf2); */
    /* load_elf32(elf); */
    /* load_elf32(elf2); */

    /* load_elf32(elf2); */

    /* jump_usermode_asm(elf->entry_point); */
    /* load_elf32(elf2); */

    /* load_elf32(elf2); */

    /* jump_usermode_asm(); */
    /* printf("after\n"); */
    /* printf("%p\n", val); */
    /* *printf("%p\n", val); */
    /* printf("%p\n", val); */
    // printf("%d\n", elf->program_headers_len);

    // char *buf = malloc(100);

    // pread(f, buf, 4, 3);

    // printf("%s\n", buf);

    /* printf("read %d bytes\n", read(f, buf, 10)); */
    /* printf("read %d bytes\n", read(f, buf + 10, 3)); */
    /* printf("read %d bytes\n", read(f, buf + 13, 5)); */
    /* printf("%s\n", buf); */

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
        /* char c = getchar(); */

        /* if (c == 'k') */
        /* { */
        /*     terminal_scroll_up(); */
        /*     continue; */
        /* } */
        /* printf("%c", c); */
    }

    return 0;
}
