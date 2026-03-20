#include "../acpi/acpica/acpi.h"
#include "multiboot.h"
#include "serial.h"
#include "task_queue.h"
#include <assert.h>
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
#include <kernel/log.h>
#include <kernel/multiboot.h>
#include <kernel/paging.h>
#include <kernel/ps2.h>
#include <kernel/semaphore.h>
#include <kernel/syscall.h>
#include <kernel/tty.h>
#include <kernel/types.h>
#include <kernel/userspace.h>
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
#include <stdbool.h>
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
    malloc(10);

    init_gdt();
    init_idt();

    serial_init(0x3f8);
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

    logi("kernel init done");

#ifdef TESTS
    run_tests();
#endif
}

WAIT_QUEUE_DEFINE(testing_wait_queue);
MUTEX_DEFINE(testing_mutex);

void *d = NULL;

void t1(void *a)
{
    /* while (true) */
    /* { */
    /*     printf("task 1 is waiting\n"); */
    /*     wait_queue_wait(&testing_wait_queue); */
    /*     printf("task 1\n"); */
    /* } */

    for (;;)
    {
        mutex_lock(&testing_mutex);

        schedule();
        memcpy(d, "one", 4);
        if (strcmp(d, "one"))
        {
            logi("one:%s\n", d);
        }

        mutex_unlock(&testing_mutex);
    }
}

void t2(void *a)
{

    /* while (true) */
    /* { */
    /*     if (!WAIT_QUEUE_EMPTY(testing_wait_queue)) */
    /*     { */
    /*         wait_queue_done(&testing_wait_queue); */
    /*         printf("task 2\n"); */
    /*     } */
    /*     else */
    /*     { */
    /*         /1* printf("task 2 not ready\n"); *1/ */
    /*     } */
    /* } */
    for (;;)
    {
        mutex_lock(&testing_mutex);

        schedule();
        memcpy(d, "two", 4);
        if (strcmp(d, "two"))
        {
            logi("two:%s\n", d);
        }

        mutex_unlock(&testing_mutex);
    }
}

void t3(void *a)
{
    while (true)
    {
        wait_queue_wait(&testing_wait_queue);
        printf("task 3\n");
    }
    for (;;)
    {
        mutex_lock(&testing_mutex);

        schedule();
        memcpy(d, "333", 4);
        if (strcmp(d, "333"))
        {
            logi("333:%s\n", d);
        }

        mutex_unlock(&testing_mutex);
    }
}

void acpica_init()
{
    ACPI_STATUS status;
    status = AcpiInitializeSubsystem();
    if (ACPI_FAILURE(status))
    {
        panic("ACPI subsystem init failed");
    }

    status = AcpiInitializeTables(NULL, 16, FALSE);
    if (ACPI_FAILURE(status))
    {
        panic("ACPI table init failed");
    }

    printf("test\n");

    status = AcpiLoadTables();
    if (ACPI_FAILURE(status))
    {
        panic("ACPI load tables failed");
    }

    status = AcpiEnableSubsystem(ACPI_FULL_INITIALIZATION);
    if (ACPI_FAILURE(status))
    {
        panic("ACPI enable subsystem failed");
    }

    status = AcpiInitializeObjects(ACPI_FULL_INITIALIZATION);
    if (ACPI_FAILURE(status))
    {
        panic("ACPI object init failed");
    }
}

struct local_apic
{
    u8 acpi_processor_id;
    u8 apic_id;
    u32 flags;
} __packed;

struct io_apic
{
    u8 apic_id;
    u8 reserved;
    u32 apic_addr;
    u32 global_interrupt_base;
};

void smp_init()
{
    ACPI_STATUS status;

    ACPI_TABLE_HEADER *table;

    status = AcpiGetTable(ACPI_SIG_MADT, 0, &table);
    if (ACPI_FAILURE(status))
    {
        panic("ACPI didnt find madt");
    }
    ACPI_TABLE_MADT *madt = (ACPI_TABLE_MADT *)table;
    ACPI_SUBTABLE_HEADER *entry;
    u8 *end = (u8 *)madt + madt->Header.Length;

    entry = (ACPI_SUBTABLE_HEADER *)(madt + 1);

    while ((u8 *)entry < end)
    {
        switch (entry->Type)
        {
        case ACPI_MADT_TYPE_LOCAL_APIC:
            printf("%d\n", entry->Length);
            printf("found an local apic\n");
            struct local_apic *local_ptr = (struct local_apic *)(entry + 1);

            printf("%d\n", local_ptr->apic_id);

            break;

        case ACPI_MADT_TYPE_IO_APIC:
            printf("found the ioapic\n");
            struct io_apic *io_ptr = (struct io_apic *)(entry + 1);
            printf("addr:%p\n", io_ptr->apic_addr);
            u32 *virtual = map_physaddr((void *)(ptr)io_ptr->apic_addr, 1,
                                        PAGING_WRITEABLE | PAGING_PRESENT);
            printf("read this:%p\n", *virtual);

            break;

        case ACPI_MADT_TYPE_INTERRUPT_OVERRIDE:
            break;
        }

        entry = (ACPI_SUBTABLE_HEADER *)((u8 *)entry + entry->Length);
    }
}

struct task_queue abc = TASK_QUEUE_INIT(abc);

ACPI_STATUS DeviceCallback(ACPI_HANDLE object, UINT32 nesting_level,
                           void *context, void **return_value)
{
    ACPI_DEVICE_INFO *info;

    if (ACPI_SUCCESS(AcpiGetObjectInfo(object, &info)))
    {

        if (info->Valid & ACPI_VALID_HID)
        {
            logd("hid: %s", info->HardwareId.String);
        }

        if (info->Valid & ACPI_VALID_UID)
        {
            logd("uid: %s", info->UniqueId.String);
        }

        if (info->Valid & ACPI_VALID_CID)
        {
            logd("compatible id");
        }

        AcpiOsFree(info);
    }

    return AE_OK;
}

void kmain()
{

    irq_disable();
    /* malloc(1000000); */
    /* malloc(1000000); */
    /* malloc(1000000); */
    acpica_init();

    AcpiWalkNamespace(ACPI_TYPE_DEVICE, ACPI_ROOT_OBJECT, 100, DeviceCallback,
                      NULL, NULL, NULL);

    smp_init();

    /* for (size_t i = 0; i < 100; i++) */
    /* { */
    /*     malloc(10); */
    /*     free(malloc(10)); */
    /* } */

    /* int fd = open("/elf_test", O_RDONLY); */
    /* size_t len = 16; */
    /* char *buf = malloc(len); */
    /* off_t offset = 16384; */
    /* pread(fd, buf, len, offset); */

    /* printf("%p\n", kernel_syscall(SYS_exit, 1, (ptr) "abc", 3, 4, 5));
     */
    /* printf("%d\n", user_addr_is_valid(kmain)); */

    /* return; */

    /* open("/one", O_RDONLY); */

    struct task *kernel_task = create_kernel_task();
    printf("this is the initial kernel task:%p\n", kernel_task);
    add_task(kernel_task);

    create_device("/terminal", get_terminal());
    create_device("/keyboard", get_keyboard());
    create_device("/mouse", mouse_get());
    create_device("/serial0", serial_cdev_create(0x3F8));

    /* struct file *read, *write; */
    /* pipe_open(&read, &write); */

    /* printf("%p, %p\n", read, write); */

    /* d = malloc(10); */
    /* ktask_run(t2, NULL); */
    /* ktask_run(t1, NULL); */
    /* /1* ktask_run(t3, NULL); *1/ */
    /* enable_scheduler(); */
    /* irq_enable(); */
    /* schedule(); */
    /* return; */

    /* int err = execv("/cat", (char *[]){"/cat", "one", NULL}); */

    logi("starting init process");

    /* int err = execv("/elf_test", (char *[]){"/elf_test", NULL}); */
    int err = execv("/wm", (char *[]){"wm", NULL});
    /* int err = execv("/ksh", (char *[]){"ksh", NULL}); */

    struct task_queue abc = TASK_QUEUE_INIT(abc);

    /* int err = execv("/cat", (char *[]){"/cat", "one", NULL}); */

    if (err < 0)
    {
        printf("did not find shell\n");
    }

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
