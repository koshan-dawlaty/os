#include "ata/ata.h"
#include "elf.h"
#include "kernel/keyboard.h"
#include "kernel/paging.h"
#include "kernel/semaphore.h"
#include "kernel/usermode.h"
#include "lib/list.h"
#include "panic.h"
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
    /* *(char *)(0xb8002) = 'a'; */
    /* *(char *)(0xb8003) = 7; */
    //     *(char *)(1) = 'a';
    return 0;
    /* asm("sti"); */

    /* /1* useless *1/ */
    /* keyboard_init(); */
    /* init_pit(); */
    /* init_pci(); */
    /* init_ata(); */

    /* init_vfs(); */
    /* init_usermode(); */
    /* serial_init(0x3F8); */

    /* test_semaphore = new_semaphore(1); */

    /* #ifdef TESTS */
    /* run_all_tests(); */
    /* #endif */

    /* char *args[] = {"/ksh", NULL}; */

    /* int f1 = open("/ksh", 0); */
    /* elf_32 *elf = parse_elf32(f1); */

    /* struct task *task1 = load_elf32(elf, 1, args); */
    /* free_elf32(elf); */

    /* struct registers *temp_regs = malloc(sizeof(struct registers)); */
    /* *temp_regs = (struct registers){0}; */

    /* enable_scheduler(); */
    /* add_task(create_kernel_task()); */

    /* /1* creat("testfile123a", 000); *1/ */

    /* /1* printf("open result:%p\n", open("/testfile123a", 0)); *1/ */

    /* /1* chdir("/dir"); *1/ */
    /* /1* printf("absolut:%p\n", lookup_path("/dir/f")); *1/ */
    /* /1* printf("relative:%s\n", lookup_path("f")->name); *1/ */
    /* /1* printf("relative:%p\n", lookup_path("one")); *1/ */

    /* add_task(task1); */
    /* printf("main exited\n"); */

    /* return 0; */
}
