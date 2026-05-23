#include "../acpi/acpica.h"
#include "multiboot.h"
#include <ata/ata.h>
#include <compiler.h>
#include <dirent.h>
#include <drivers/block/ram/ramdisk.h>
#include <drivers/character/keyboard.h>
#include <drivers/character/mouse.h>
#include <drivers/character/serial.h>
#include <drivers/character/terminal.h>
#include <endian.h>
#include <err_ptr.h>
#include <errno.h>
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
#include <kernel/stacktrace.h>
#include <kernel/syscall.h>
#include <kernel/tty.h>
#include <kernel/types.h>
#include <kernel/userspace.h>
#include <lib/array.h>
#include <lib/assert.h>
#include <lib/binary_tree.h>
#include <lib/list.h>
#include <lib/log.h>
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
#include <serial.h>
#include <socket.h>
#include <spinlock.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <task_queue.h>
#include <userspace/mouse.h>
#include <utils.h>
#include <vfs.h>
#include <wait_queue.h>

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
            /* framebuffer_put_pixel(f, color, x, y); */
            /* framebuffer_put_pixel(f, (x + y) % 2 == 0 ? 0xFFFFFF : 0, x, y);
             */
            framebuffer_put_pixel(f, 0x0F0F0F, x, y);
        }
    }

    /* framebuffer_draw_rect(f, 600, 200, 100, 100, 0xff0000); */
    /* framebuffer_draw_rect(f, 650, 250, 100, 100, 0x00ff00); */
    /* framebuffer_draw_rect(f, 700, 300, 100, 100, 0x0000ff); */
}

void sse_enable()
{
    ptr cr0 = 0;
    /* read from cr0 */
    asm volatile("mov %%cr0, %0" : "=r"(cr0) :);

    cr0 &= ~(1 << 2); /* unset EM */
    cr0 |= (1 << 5);  /* set NE */
    cr0 |= (1 << 1);  /* set MP */

    /* write to cr0 */
    asm volatile("mov %%cr0, %0" : "=r"(cr0) :);

    /* TODO: check for long mode / sse support for these */
    ptr cr4 = 0;
    asm volatile("mov %%cr4, %0" : "=r"(cr4) :);

    cr4 |= (1 << 9); /* set OSFXSR */

    /* might cause issues with exceptions */
    cr4 |= (1 << 10); /* set OSXMMEXCPT */

    /* TODO: needs a cpuid check */
    /* cr4 |= (1 << 18); /1* set OSXSAVE *1/ */

    asm volatile("mov %0, %%cr4" : : "r"(cr4));

    asm("fninit");
}

extern __int_handler void syscall_handler_asm(struct interrupt_frame *frame);
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

    init_gdt();
    init_idt();

    /* volatile char *addr = get_free_pages_zone(1, &gfp_kernel_tree_zone); */
    /* printf("memory allocated\n"); */
    /* printf("at address:%p\n", addr); */
    /* *addr = 'a'; */
    /* if (c != 'a') */
    /* { */
    /*     printf("SJLKDFLSKDFJLSDFJ\n"); */
    /* } */
    /* printf("wrote to memory\n"); */
    /* char c = *addr; */
    /* printf("got something from memory\n"); */
    /* printf("read:%c\n", c); */
    /* irq_disable(); */
    /* irq_wait(); */

#ifndef CONFIG_PLEASE_DONT_USE_THE_SERIAL_PORT
    /* serial_init(0x3f8); */
#endif
    /* irq_enable(); */
    /* char *str = "asjdfakldf"; */
    /* serial_writestring(str, strlen(str), 0x3f8); */
    /* for (;;) */
    /* { */
    /*     u8 c = serial_read(0x3f8); */
    /*     printf("%c\n", c); */
    /* } */

    /* useless */
    keyboard_init();

    init_pit();
    init_pci();
    init_ata();

    init_vfs();

    ps2_init();

    call_constructors();

    IRQ_OFF({
        register_interrupt_handler(0x80, syscall_handler_asm,
                                   (int_handler_flags){.is_user = true});
    })

    /* TODO: put in arch init */
    sse_enable();

    logi("kernel init done");

#ifdef TESTS
    run_tests();
#else
    symbol_table_parse(); /* seems to break with tests */
#endif
}

WAIT_QUEUE_DEFINE(poll_wait_queue);

void t2(void *a)
{
    struct file *f = a;
    logd("t2");
}

void t3(void *a)
{
    logd("t3");
    struct file *f = a;
    logd("writing");
    write_file(f, "abc", 4, 0);
}

void t1(void *a)
{
}

void ls(const char *path)
{
    DIR *d = opendir(path);

    struct dirent *e = readdir(d);
    while (e != NULL)
    {
        logd("%s", e->d_name);
        e = readdir(d);
    }
}

struct task_queue abc = TASK_QUEUE_INIT(abc);
extern void scheduler_init();

void test1()
{
    int write_only = open("/one", O_WRONLY);
    char *buf = malloc(100);
    int err = read(write_only, buf, 100);
    close(write_only);
    logd("done close");
    int read_only = open("/one", O_RDONLY);
    logd("done open");
    err = write(read_only, buf, 100);
    logd("done write");
    close(read_only);
}

void kmain()
{

    /* return; */
    scheduler_init();

    irq_disable();
#ifdef CONFIG_ACPICA
    acpica_init();
#endif
    /* smp_init(); */

    struct task *kernel_task = create_kernel_task();
    printf("this is the initial kernel task:%p\n", kernel_task);
    add_task(kernel_task);

    create_device("/dev/terminal", get_terminal());
    create_device("/dev/keyboard", get_keyboard());
    create_device("/dev/mouse", mouse_get());
#ifndef CONFIG_PLEASE_DONT_USE_THE_SERIAL_PORT
    create_device("/dev/serial0", serial_cdev_create(0x3F8));
#endif

    /* ktask_run(t1, NULL); */
    /* irq_enable(); */
    /* enable_scheduler(); */
    /* return; */

    /* struct file *read, *write; */
    /* pipe_open(&read, &write); */

    /* printf("%p, %p\n", read, write); */

    /* int err = execv("/elf_test", (char *[]){"/elf_test", NULL}); */

    int err = execv("/bin/wm", (char *[]){"wm", NULL});
    /* int err = execv("/ksh", (char *[]){"ksh", NULL}); */
    /* int err = execv("/bin/cat", (char *[]){"/bin/cat", "one", NULL}); */
    /* int err = execv("/bin/ls", (char *[]){"/bin/ls", "/dev", NULL}); */
    /* int err = execv("/cat", (char *[]){"/cat", "one", NULL}); */

    enable_scheduler();
    irq_enable();
    /* schedule(); */
    /* execv("/elf_test", (char *[]){"elf_test", NULL}); */
    /* execv("/elf_test", (char *[]){"elf_test", NULL}); */

    /* execv("imgview", (char *[]){"imgview", "osdev.tga", NULL}); */
    /* execv("imgview", (char *[]){"imgview", "dog.tga", NULL}); */
    /* while (1) */
    /* { */
    /*     schedule(); */
    /* } */
}
