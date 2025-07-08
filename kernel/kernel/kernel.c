#include "ata/ata.h"
#include "elf.h"
#include "kernel/keyboard.h"
#include "kernel/paging.h"
#include "kernel/usermode.h"
#include "panic.h"
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
#include "tests/tests.h"
#include "unistd.h"

extern struct pagedir init_pgdir;

int sakdjflaskdflkas()
{
    return 1234567;
}
int sakdjflaskdflkas2()
{
    return 1234567;
}
int sakdjflaskdflkas22()
{
    return 1234567;
}
int sakdjflaskdflkas223()
{
    return 1234567;
}
int sakdjflaskdflkas245q2()
{
    return 1234567;
}
int sakdjflaskdflkas245q2aksdf()
{
    return 1234567;
}

int sakdjflaskdflkas245q2sadf()
{
    return 1234567;
}

int sakdjflaskdflkas245q2sdf()
{
    return 1234567;
}

int sakdjflaskdflkas245q2sdfjdsaflkjdf()
{
    return 1234567;
}

int sakdjflaskdflkas245q2sdfsdjkfasjkld()
{
    return 1234567;
}

int sakdjflaskdflkas245q2sdfsdjkfasjkldjlksdafjaksdf()
{
    return 1234567;
}

int sakdjflaskdflkas245q2sdfsdjkfasjkldjlksdafjaksdfjklsdajfasldfjs()
{
    return 1234567;
}

int sakdjflaskdflkas245q2sdfsdjkfasjkldjlksdafjaksdfjklsdajfasldfjskljdsfjalsdfjasldfajsdflaskdjfas()
{
    return 1234567;
}

int jasldkfjsakdjflaskdflkas245q2sdfsdjkfasjkldjlksdafjaksdfjklsdajfasldfjs()
{
    return 1234567;
}

int sakdjflaskdflkas245q2sdfsdjkfasjkldjlksdafjaksdfjklsdajfasldfjsjlsdfjasdflkajsdlfk()
{
    return 1234567;
}

int asjdflasdkjfalkdfjalksdfljkasdflkjasdfsakdjflaskdflkas245q2sdfsdjkfasjkldjlksdafjaksdfjklsdajfasldfjs()
{
    return 1234567;
}

int jalsdfjasdfsakdjflaskdflkas245q2sdfsdjkfasjkldjlksdafjaksdfjklsdajfasldfjs()
{
    return 1234567;
}

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

    /* useless */
    /* keyboard_init(); */
    init_pit();
    init_pci();
    init_ata();
    init_vfs();
    init_usermode();
#ifdef TESTS
    run_all_tests();
#endif

    int f1 = open("/elf_test", 0);
    elf_32 *elf = parse_elf32(f1);
    struct pagedir *dir1 = load_elf32(elf);

    printf("test\n");

    int f2 = open("/elf_test2", 0);
    elf_32 *elf2 = parse_elf32(f2);
    struct pagedir *dir2 = load_elf32(elf2);

    /* int f3 = open("/usr/bin/elf_test", 0); */
    /* elf_32 *elf3 = parse_elf32(f3); */
    /* struct pagedir *dir3 = load_elf32(elf3); */

    enable_scheduler();
    load_pgdir(dir1);

    jump_usermode_asm(elf->entry_point);

    printf("main exited\n");
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
