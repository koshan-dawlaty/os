#include "multiboot.h"
#include "wait_queue.h"
#include <assert.h>
#include <ata/ata.h>
#include <compiler.h>
#include <dirent.h>
#include <drivers/block/ram/ramdisk.h>
#include <drivers/character/keyboard.h>
#include <drivers/character/mouse.h>
#include <drivers/character/terminal.h>
#include <endian.h>
#include <err.h>
#include <fcntl.h>
#include <gdt.h>
#include <interrupts/idt.h>
#include <kernel/addr_space.h>
#include <kernel/graphics.h>
#include <kernel/interrupts.h>
#include <kernel/keyboard.h>
#include <kernel/multiboot.h>
#include <kernel/paging.h>
#include <kernel/ps2.h>
#include <kernel/semaphore.h>
#include <kernel/syscall.h>
#include <kernel/tty.h>
#include <kernel/types.h>
#include <lib/array.h>
#include <lib/binary_tree.h>
#include <lib/list.h>
#include <malloc_internal.h>
#include <memory/get_free_pages.h>
#include <memory/paging_flags.h>
#include <mutex.h>
#include <panic.h>
#include <pci/pci.h>
#include <pipe.h>
#include <pit/pit.h>
#include <sched.h>
#include <scheduler.h>
#include <socket.h>
#include <spinlock.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <userspace/mouse.h>
#include <utils.h>
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
            /* framebuffer_put_pixel(f, (x + y) % 2 == 0 ? 0xFFFFFF : 0, x, y);
             */
            /* framebuffer_put_pixel(f, 0xFFFFFF, x, y); */
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

    ps2_init();

    call_constructors();

#ifdef TESTS
    run_tests();
#endif
}

struct mutex testing_mutex = MUTEX_INIT();
struct wait_queue testing_wait_queue = WAIT_QUEUE_INIT();

void t1(void *a)
{
    struct file *pipe = a;
    write_file(pipe, "abc", 4, 0);
}

void t2(void *a)
{
    struct file *pipe = a;
    char *buf = malloc(4);
    read_file(pipe, buf, 4, 0);
    printf("read:%s\n", buf);
}

void t3(void *a)
{
    int m = open("/mouse", O_RDONLY);
    struct mouse_event *ev = malloc(sizeof(struct mouse_event));
    read(m, ev, sizeof(struct mouse_event));
    printf("x change: %d\n", ev->x_change);
}

extern __int_handler void syscall_handler_asm(struct interrupt_frame *frame);

void kmain()
{

    IRQ_OFF({
        register_interrupt_handler(0x80, syscall_handler_asm,
                                   (int_handler_flags){.is_user = true});
    })

    struct addr_space addr_sp = addr_space_alloc();
    /* printf("%p\n", addr_sp.dir); */
    /* addr_space_load(addr_sp); */

    /* void *addr = (void *)0xB00000000; */

    /* addr_space_map(&addr_sp, addr, 10, PAGING_PRESENT | PAGING_WRITEABLE); */
    /* for (size_t i = 0; i < 100; i++) */
    /* { */
    /*     ((char *)(addr + 9 * PAGE_SIZE))[i] = (i % 10 + 0x30); */
    /* } */

    /* printf("%s\n", addr + 9 * PAGE_SIZE); */

    /* addr_space_unmap(&addr_sp, addr, 10); */

    for (size_t i = 0; i < memory_regions_len(); i++)
    {
        struct memory_region region = memory_regions_get(i);
        printf("memory from: %p-%p\n", region.start, region.start + region.len);
    }

    printf("%p\n", malloc(10));

    /* open("/one", O_RDONLY); */

    struct task *kernel_task = create_kernel_task();
    add_task(kernel_task);

    create_device("/terminal", get_terminal());
    create_device("/keyboard", get_keyboard());
    create_device("/mouse", mouse_get());

    /* struct file *read, *write; */
    /* pipe_open(&read, &write); */

    /* printf("%p, %p\n", read, write); */

    /* ktask_run(t2, read); */
    /* ktask_run(t1, write); */
    /* ktask_run(t3, NULL); */
    /* enable_scheduler(); */
    /* irq_enable(); */
    /* schedule(); */
    /* return; */

    /* int err = execv("/cat", (char *[]){"/cat", "one", NULL}); */

    printf("starting shell\n");
    execv("/elf_test", (char *[]){"elf_test", NULL});
    /* int err = execv("/elf_test", (char *[]){"/elf_test", NULL}); */
    int err = execv("/wm", (char *[]){"wm", NULL});
    /* int err = execv("/ksh", (char *[]){"ksh", NULL}); */

    if (err < 0)
    {
        printf("did not find shell\n");
    }

    enable_scheduler();
    irq_enable();
    schedule();
    /* execv("/elf_test", (char *[]){"elf_test", NULL}); */
    /* execv("/elf_test", (char *[]){"elf_test", NULL}); */
    while (1)
    {
        schedule();
    }
}
