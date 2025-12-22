#include "kernel/graphics.h"
#include "kernel/types.h"
#include "lib/binary_tree.h"
#include "multiboot.h"
#include <assert.h>
#include <ata/ata.h>
#include <dirent.h>
#include <drivers/block/ram/ramdisk.h>
#include <drivers/character/keyboard.h>
#include <drivers/character/terminal.h>
#include <err.h>
#include <fcntl.h>
#include <gdt.h>
#include <interrupts/idt.h>
#include <kernel/keyboard.h>
#include <kernel/paging.h>
#include <kernel/semaphore.h>
#include <kernel/tty.h>
#include <malloc_internal.h>
#include <memory/get_free_pages.h>
#include <memory/paging_flags.h>
#include <panic.h>
#include <pci/pci.h>
#include <pit/pit.h>
#include <scheduler.h>
#include <spinlock.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vfs.h>

#include <scheduler.h>
#include <unistd.h>

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
spinlock *shared_memory_spinlock;
char *shared;
void kernel_task1(void *args)

{
    printf("got:%s\n", args);
    printf("1 is running\n");
    sleep(1);
    printf("this is kernel task1\n");
    while (1)
    {
        lock_spinlock(shared_memory_spinlock);
        memcpy(shared, "one", 4);

        schedule();
        if (strcmp(shared, "one"))
        {
            printf("task1: shared memory was corrupted to:%s\n", shared);
        }

        release_spinlock(shared_memory_spinlock);

        schedule();
    }
}

void kernel_task2(void *args)
{
    printf("2 is running\n");
    sleep(2);
    printf("this is kernel task2\n");
    while (1)
    {

        lock_spinlock(shared_memory_spinlock);

        memcpy(shared, "two", 4);

        schedule();
        schedule();
        if (strcmp(shared, "two"))
        {
            printf("task2: shared memory was corrupted to:%s\n", shared);
        }

        release_spinlock(shared_memory_spinlock);

        schedule();
    }
}

void kernel_task3(void *args)
{

    printf("this is kernel task3\n");
    while (1)
    {

        printf("in kernel_task3\n");
        char *x = malloc(10);
        schedule();

        memcpy(x, "one", 4);

        schedule();

        if (strcmp(x, "one"))
        {
            printf("task1: shared memory was corrupted to:%s\n", x);
        }

        schedule();
        free(x);
        schedule();
    }
}

void kernel_task4(void *args)
{
    printf("this is kernel task4\n");
    while (1)
    {
        printf("in kernel_task4\n");

        char *x = malloc(10);
        schedule();
        memcpy(x, "two", 4);
        schedule();
        if (strcmp(x, "two"))
        {
            printf("task2: shared memory was corrupted to:%s\n", x);
        }

        schedule();
        free(x);
        schedule();
    }
}

void kernel_task5(void *args)
{
    printf("this is kernel task5\n");
    while (1)
    {

        printf("in kernel_task5\n");
        schedule();

        char *x = malloc(10);
        schedule();
        memcpy(x, "333", 4);
        schedule();
        if (strcmp(x, "333"))
        {
            printf("task3: shared memory was corrupted to:%s\n", shared);
        }

        schedule();
        free(x);
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

struct framebuffer *f;
extern struct framebuffer *tty_framebuffer;
void draw_background(struct framebuffer *f)
{
    for (size_t y = 0; y < f->height; y++)
    {
        for (size_t x = 0; x < f->width; x++)
        {
            /* u8 r = x; */
            /* u8 g = y; */
            /* u8 b = x * y; */

            u8 r = (x & y) ^ 43;
            u8 g = 0;
            u8 b = x * y;

            int color = r | (g << 8) | (b << 16);
            framebuffer_put_pixel(f, color, x, y);
        }
    }

    framebuffer_draw_rect(f, 600, 200, 100, 100, 0xff0000);
    framebuffer_draw_rect(f, 650, 250, 100, 100, 0x00ff00);
    framebuffer_draw_rect(f, 700, 300, 100, 100, 0x0000ff);
}

void kernel_init()
{

    terminal_initialize();
    init_paging();
    init_heap();
    init_gfp_zones();

    f = multiboot_init();
    if (f != NULL)
    {
        terminal_init_graphics(f);
        graphics_init(f);
        draw_background(f);
    }

    malloc(10);

    init_gdt();
    init_idt();

    /* useless */
    keyboard_init();

    init_pit();
    init_pci();
    init_ata();

    init_vfs();

    call_constructors();

    shared_memory_semaphore = new_semaphore(1);
    shared_memory_spinlock = new_spinlock();

    create_device("/terminal", get_terminal());
    create_device("/keyboard", get_keyboard());
    add_task(create_kernel_task());
    /*
    run_task(kernel_task3, NULL);
    run_task(kernel_task4, NULL);
    run_task(kernel_task5, NULL);
    */

#ifdef TESTS
    run_tests();
#endif
}

int main()
{

    /* creat("abc", 0); */
    /* creat("two", 0); */

    /* printf("ls\n"); */
    /* DIR *d = opendir("/"); */
    /* struct dirent *ent = NULL; */
    /* do */
    /* { */
    /*     ent = readdir(d); */
    /*     printf("%s\n", ent->d_name); */
    /* } while (ent != NULL); */
    /* printf("ls end\n"); */

    /* int fd = open("/abc", 0); */

    /* pwrite(fd, "abcdefg", 8, 0); */
    /* char *buf = malloc(100); */
    /* pread(fd, buf, 100, 0); */

    /* printf("read:%s\n", buf); */

    printf("starting shell\n");
    int err = execv("/ksh", (char *[]){NULL});
    if (err < 0)
    {
        printf("did not find shell\n");
    }
    enable_scheduler();

    return 0;
}
