#include "ata/ata.h"
#include "elf.h"
#include "gdt.h"
#include "interrupts/idt.h"
#include "kernel/keyboard.h"
#include "kernel/paging.h"
#include "kernel/semaphore.h"
#include "kernel/usermode.h"
#include "kernel/utils.h"
#include "lib/list.h"
#include "panic.h"
#include "pit/pit.h"
#include "serial.h"
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

    init_gdt();
    init_idt();

    /* get_free_pages(1); */

    map_page((void *)0x12345678, (void *)0x123466, 0);

    printf("this is a number:%d\n", 123);

    keyboard_init();

    init_pit();
    init_pci();
    init_ata();
    init_vfs();

    /* asm("int $(0x50)"); */
    /* printf("test:%p\n", malloc(1)); */

    /* printf("%p\n", *(char *)get_free_pages(1)); */

    /* init_usermode(); */
    /* serial_init(0x3F8); */

    /* test_semaphore = new_semaphore(1); */

    /* #ifdef TESTS */
    /* run_all_tests(); */
    /* #endif */

    char *args[] = {"/ksh", NULL};

    int f1 = open("/ksh", 0);
    elf *elf = parse_elf(f1);

    struct task *task1 = load_elf(elf, 1, args);

    free_elf(elf);

    enable_scheduler();
    add_task(create_kernel_task());
    add_task(task1);

    return 0;
}
