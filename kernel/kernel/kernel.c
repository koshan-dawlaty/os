#include "kernel/types.h"
#include "lib/binary_tree.h"
#include "multiboot.h"
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

void kernel_init()
{

    terminal_initialize();
    init_paging();
    init_heap();
    init_gfp_zones();

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

#ifdef TESTS
    run_tests();
#endif
}

/* we can assume these are next to each other, struct then magic */
extern u32 multiboot_struct_physical;
extern u32 multiboot_magic_physical;

/* static void putpixel(unsigned char *screen, int x, int y, int color) */
/* { */
/*     /1* unsigned where = x * 3 + y * pitch; *1/ */
/*     /1* screen[where] = color & 255;             // BLUE *1/ */
/*     /1* screen[where + 1] = (color >> 8) & 255;  // GREEN *1/ */
/*     /1* screen[where + 2] = (color >> 16) & 255; // RED *1/ */
/* } */

void multiboot_init()
{
    void *virtual = get_free_pages_unmmaped(1, &gfp_kernel_zone);
    map_page(&multiboot_struct_physical, virtual,
             PAGING_PRESENT | PAGING_WRITEABLE);

    u32 multiboot_struct_addr = *(u32 *)(virtual);
    u32 multiboot_magic = *((u32 *)(virtual) + 1);

    printf("magic:%ux\n", multiboot_magic);

    struct multiboot_info *info_physical = (void *)(ptr)multiboot_struct_addr;

    printf("info addr:%p\n", info_physical);

    struct multiboot_info *info = get_free_pages_unmmaped(1, &gfp_kernel_zone);
    map_page((void *)(ptr)multiboot_struct_addr, info,
             PAGING_PRESENT | PAGING_WRITEABLE);

    printf("%d\n", offsetof(struct multiboot_info, framebuffer_height));

    if (info->flags & (1 << 12))
    {
        printf("good\n");
        printf("framebuffer addr: %p\n", info->framebuffer_addr);
        /* printf("width x height: %u x %u\n", info->framebuffer_width, */
        /*        info->framebuffer_height); */
        /* printf("bpp: %u\n", info->framebuffer_bpp); */
    }
    if (info->framebuffer_type != 1)
    {

        return;
    }

    printf("framebuffer:%p\n", info->framebuffer_addr);

    /* size_t total_size = info->framebuffer_width * (info->framebuffer_bpp / 8)
     * * */
    /*                     info->framebuffer_height * info->framebuffer_pitch;
     */
    size_t total_size = info->framebuffer_pitch * info->framebuffer_height;

    void *framebuffer =
        get_free_pages_unmmaped((total_size / 4096) + 1, &gfp_kernel_zone);

    map_page((void *)(ptr)info->framebuffer_addr, framebuffer,
             PAGING_PRESENT | PAGING_WRITEABLE);

    printf("pitch:%p\n", info->framebuffer_pitch);
    int y = 0;
    int x = 10;
    /* unsigned char *pixel = */
    /*     framebuffer + y * info->framebuffer_pitch + x *
     * info->framebuffer_bpp; */

    size_t location =
        y * info->framebuffer_pitch + x * (info->framebuffer_bpp / 8);

    for (size_t i = 0; i < 100; i++)
    {
        int color = 0xff00ff;
        unsigned char *screen = framebuffer;
        screen[location] = color & 255;             // BLUE
        screen[location + 1] = (color >> 8) & 255;  // GREEN
        screen[location + 2] = (color >> 16) & 255; // RED
        location += info->framebuffer_bpp / 8;
    }
    /* memset(framebuffer, 'a', 100); */
}

int main()
{

    multiboot_init();

    /* create_device("/term", get_terminal()); */
    /* file *d = open_device("/term"); */

    /* write_file(d, "abc", 4, 0); */

    /* return 1; */

    /* int f1 = open("/one", 0); */
    /* int f2 = open("/one", 0); */

    /* close(f1); */
    /* close(f2); */

    /* return 1; */
    /* printf("IN MAIN\n"); */

    /* shared = malloc(10); */
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
    enable_scheduler();

    int err = execv("/ksh", (char *[]){NULL});
    if (err < 0)
    {
        printf("did not find shell\n");
    }

    return 0;
}
