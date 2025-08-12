#include "ata/ata.h"
#include "elf.h"
#include "gdt.h"
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
#include "semaphore.h"
#include "tests/tests.h"
#include "unistd.h"

#include <stdint.h>
#include <stdlib.h>

#if UINT32_MAX == UINTPTR_MAX
/* #define STACK_CHK_GUARD 0xe2dee396 */
#define STACK_CHK_GUARD 0xBADC0DE
#else
#define STACK_CHK_GUARD 0x595e9fbd94fda766
#endif

uintptr_t __stack_chk_guard = STACK_CHK_GUARD;

__attribute__((noreturn)) void __stack_chk_fail(void)
{
    panic("stack smashing detected");
    while (1)
    {
    }
}

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
    asm("cli");

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
    struct task *task1 = load_elf32(elf);

    int f2 = open("/elf_test2", 0);
    elf_32 *elf2 = parse_elf32(f2);
    struct task *task2 = load_elf32(elf2);

    struct registers *temp_regs = malloc(sizeof(struct registers));
    *temp_regs = (struct registers){0};

    /* printf("test:%d\n", 1234); */

    /* load_pgdir(task1->pgdir); */
    /* printf("setting tss.esp0 to:%p\n", task1->stack); */
    /* set_kernel_stack((uint32_t)task1->stack); */

    enable_scheduler();
    /* load_registers(&task1->regs); */
    add_task(create_kernel_task());

    add_task(task1);
    add_task(task2);
    /* jump_usermode_asm(elf->entry_point, temp_regs); */

    printf("main exited\n");

    return 0;
}
