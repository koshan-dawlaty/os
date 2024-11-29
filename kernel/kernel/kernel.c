#include "multiboot.h"
#include "paging.h"
#include <kernel/tty.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

extern int _kernel_end;

int main(multiboot_info_t *mbd, unsigned int magic) {
    terminal_initialize();

    init_paging(mbd);

    /* if (magic != MULTIBOOT_BOOTLOADER_MAGIC) { */
    /*     printf("invalid bootloader magic!\n"); */
    /*     return -1; */
    /* } */

    /* printf("%p\n", mbd); */
    /* printf("%p\n", mbd->flags); */
    /* if (!(mbd->flags >> 6 & 0x1)) { */
    /*     printf("invalid memory map given by GRUB bootloader\n"); */
    /* } */

    /* init_paging(mbd); */

    /* printf("%p\n", mbd->mmap_addr); */

    /* /1* printf("%c %s %d %d def\n", 'a', "bc", 1234, -5678); *1/ */

    /* unsigned int i; */

    /* for (i = 0; i < mbd->mmap_length; i += sizeof(multiboot_memory_map_t)) {
     */
    /*     multiboot_memory_map_t *mmmt = */
    /*         (multiboot_memory_map_t *)(mbd->mmap_addr + i); */

    /*     if (i > 0) { */
    /*         break; */
    /*     } */

    /*     printf("asfjdksakdjf\n"); */
    /*     /1* printf("Start Addr: %p | Length: %p | Size: %p | Type: %d\n", *1/
     */
    /*     /1*        mmmt->addr, mmmt->len, mmmt->size, mmmt->type); *1/ */
    /*     /1* printf("Start Addr: %p\n", mmmt->addr); *1/ */

    /*     if (mmmt->type == MULTIBOOT_MEMORY_AVAILABLE) { */
    /*         /* */
    /*          * Do something with this memory block! */
    /*          * BE WARNED that some of memory shown as availiable is */
    /*          * actually actively being used by the kernel! You'll need to */
    /*          * take that into account before writing to memory! */
    /*          *1/ */
    /*     } */
    /* } */

    return 0;
}
