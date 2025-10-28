#include "ata/ata.h"
#include "elf.h"
#include "kernel/keyboard.h"
#include "kernel/paging.h"
#include "kernel/semaphore.h"
#include "kernel/usermode.h"
#include "lib/list.h"
#include "panic.h"
#include "pci/pci.h"
#include "pit/pit.h"
#include "scheduler.h"
#include "scheduler/sleep.h"
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

semaphore *shared_memory_semaphore;
char *shared;
void kernel_task1()
{
    printf("1 is running\n");
    sleep(1);
    printf("this is kernel task1\n");
    while (1)
    {
        wait_semaphore(shared_memory_semaphore);
        memcpy(shared, "one", 4);

        schedule();
        if (strcmp(shared, "one"))
        {
            printf("task1: shared memory was corrupted to:%s\n", shared);
        }

        release_semaphore(shared_memory_semaphore);

        schedule();
    }
}

void kernel_task2()
{
    printf("2 is running\n");
    sleep(2);
    printf("this is kernel task2\n");
    while (1)
    {

        wait_semaphore(shared_memory_semaphore);

        memcpy(shared, "two", 4);

        schedule();
        if (strcmp(shared, "two"))
        {
            printf("task2: shared memory was corrupted to:%s\n", shared);
        }

        release_semaphore(shared_memory_semaphore);

        schedule();
    }
}

void kernel_task3()
{
    printf("3 is running\n");
    sleep(3);

    printf("this is kernel task3\n");
    while (1)
    {
    }
}

int main()
{
    asm("sti");

    printf("abc:%p\n", main);
    printf("abcdef:%p\n", 2);

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
    free_elf32(elf);

    struct registers *temp_regs = malloc(sizeof(struct registers));
    *temp_regs = (struct registers){0};

    enable_scheduler();
    add_task(create_kernel_task());

    /* add_task(task1); */

    shared = malloc(10);
    shared_memory_semaphore = new_semaphore(1);

    add_task(new_task((ptr)kernel_task1));
    add_task(new_task((ptr)kernel_task2));
    add_task(new_task((ptr)kernel_task3));

    return 0;
}
