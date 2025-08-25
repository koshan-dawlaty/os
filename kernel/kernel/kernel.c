#include "ata/ata.h"
#include "elf.h"
#include "kernel/keyboard.h"
#include "kernel/paging.h"
#include "kernel/semaphore.h"
#include "kernel/usermode.h"
#include "panic.h"
#include "pci/pci.h"
#include "pit/pit.h"
#include "serial.h"
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
#include "scheduler.h"
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

extern semaphore *test_semaphore;

int main()
{
    asm("sti");

    /* useless */
    keyboard_init();
    init_pit();
    init_pci();
    init_ata();
    init_vfs();
    init_usermode();
    serial_init(0x3F8);

    test_semaphore = new_semaphore(1);

#ifdef TESTS
    run_all_tests();
#endif

    char *args[] = {"/ksh", NULL};

    int f1 = open("/ksh", 0);
    elf_32 *elf = parse_elf32(f1);
    struct task *task1 = load_elf32(elf, 1, args);

    /* int f2 = open("/elf_test2", 0); */
    /* elf_32 *elf2 = parse_elf32(f2); */
    /* struct task *task2 = load_elf32(elf2, 0, args); */

    free_elf32(elf);
    /* free_elf32(elf2); */

    printf("STARTING\n");

    int fd = open("/one", 0);
    char *literal = "abcdefg";
    char *wbuf = malloc(strlen(literal) + 1);
    wbuf[strlen(literal)] = 0;
    memcpy(wbuf, literal, strlen(literal) + 1);
    write(fd, wbuf, strlen(literal) + 1);

    char *rbuf = malloc(100);
    pread(fd, rbuf, 100, 0);
    for (size_t i = 0; i < 10; i++)
    {
        printf("%p\n", rbuf[i]);
    }
    printf("buf:%s\n", rbuf);

    struct registers *temp_regs = malloc(sizeof(struct registers));
    *temp_regs = (struct registers){0};

    enable_scheduler();
    add_task(create_kernel_task());

    add_task(task1);
    /* add_task(task2); */

    printf("main exited\n");

    return 0;
}
