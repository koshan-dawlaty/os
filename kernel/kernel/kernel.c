#include "ata/ata.h"
#include "elf.h"
#include "gdt.h"
#include "interrupts/idt.h"
#include "kernel/keyboard.h"
#include "kernel/paging.h"
#include "kernel/semaphore.h"
#include "kernel/utils.h"
#include "lib/list.h"
#include "memory/get_free_pages.h"
#include "panic.h"
#include "pit/pit.h"
#include "scheduler.h"
#include "scheduler/sleep.h"
#include "serial.h"
#include <drivers/block/ram/ramdisk.h>
#include <drivers/character/terminal/terminal.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <kernel/tty.h>
#include <memory/paging_flags.h>
#include <pci/pci.h>
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
#include "utils.h"

#include <stdint.h>
#include <stdlib.h>
#include <tests/tests.h>

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

    printf("this is kernel task3\n");
    while (1)
    {
        char *x = malloc(100);
        schedule();

        memcpy(x, "one", 4);

        schedule();

        if (strcmp(x, "one"))
        {
            printf("task1: shared memory was corrupted to:%s\n", shared);
        }

        schedule();
        free(x);
        schedule();
        schedule();
    }
}

void kernel_task4()
{
    printf("this is kernel task4\n");
    while (1)
    {

        char *x = malloc(100);
        schedule();

        memcpy(x, "two", 4);
        schedule();
        if (strcmp(x, "two"))
        {
            printf("task2: shared memory was corrupted to:%s\n", shared);
        }

        schedule();
        free(x);
        schedule();
        schedule();
    }
}

extern void (*__ctors_start[])(void);
extern void (*__ctors_end[])(void);

void call_constructors(void)
{

    for (void (**ctor)(void) = __ctors_start; ctor < __ctors_end; ++ctor)
    {
        (*ctor)();
    }
}

void kernel_init()
{

    terminal_initialize();
    init_paging();
    init_gfp_zones();

    init_gdt();
    init_idt();

    /* useless */
    keyboard_init();

    init_pit();
    init_pci();
    init_ata();

    init_vfs();

    call_constructors();

#ifdef TESTS
    run_all_tests();
#endif
}

int main()
{

    printf("IN MAIN\n");

    char *args[] = {"/ksh", NULL};

    int f1 = open("/ksh", 0);

    if (f1 < 0)
    {
        panic("could not open shell");
    }

    elf *elf = parse_elf(f1);

    struct task *task1 = load_elf(elf, 1, args);

    free_elf(elf);

    enable_scheduler();
    add_task(create_kernel_task());

    /* add_task(task1); */

    shared = malloc(10);
    shared_memory_semaphore = new_semaphore(1);

    add_task(task1);

    create_device("/terminal", get_terminal());
    /* int fd = open("/terminal", 0); */

    /* char *buf = malloc(10); */
    /* read(fd, buf, 10); */
    /* printf("%s\n", buf); */

    /* add_task(new_task((ptr)kernel_task3)); */
    /* add_task(new_task((ptr)kernel_task4)); */

    return 0;
}
