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

#ifdef TESTS
    run_tests();
#endif
}

int main()
{

    /* creat("two", 0); */

    printf("ls\n");
    DIR *d = opendir("/");
    struct dirent *ent = NULL;
    do
    {
        ent = readdir(d);
        printf("read:%s\n", ent->d_name);
    } while (ent != NULL);
    printf("ls end\n");

    /* int fd = open("/abc", 0); */

    /* pwrite(fd, "abcdefg", 8, 0); */
    /* char *buf = malloc(100); */
    /* pread(fd, buf, 100, 0); */

    /* printf("read:%s\n", buf); */

    creat("/tmpfs/b", 0);
    int fd = open("/tmpfs/b", 0);
    char *buf = malloc(100);
    pwrite(fd, "abc", 4, 0);
    pread(fd, buf, 100, 0);
    printf("file one:%s\n", buf);

    create_device("/terminal", get_terminal());
    create_device("/keyboard", get_keyboard());

    add_task(create_kernel_task());
    printf("starting shell\n");
    int err = execv("/ksh", (char *[]){"ksh", NULL});
    getchar();

    /* printf("process 1\n"); */
    /* execv("cat", (char *[]){"cat", "one", NULL}); */
    /* getchar(); */
    /* printf("process 2\n"); */
    /* execv("cat", (char *[]){"cat", "two", NULL}); */

    if (err < 0)
    {
        printf("did not find shell\n");
    }

    enable_scheduler();

    return 0;
}
