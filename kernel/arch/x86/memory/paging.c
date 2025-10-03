#include "kernel/paging.h"
#include "gdt.h"
#include "interrupts/idt.h"
#include "kernel/tty.h"
#include "kernel/utils.h"
#include "memory/paging_flags.h"
#include "memory/paging_internal.h"

#include "multiboot.h"
#include "vga.h"

#include <kernel/paging.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint32_t directory_entry;
typedef uint32_t table_entry;

extern long paging_page_table_addr;
#define paging_page_table ((table_entry *)&paging_page_table_addr)

#define PAGE_SIZE 4096

#ifdef CONFIG_x86_64
#define DIRECTORY_ADDR ((directory_entry *)0xffffc00000001000)

#elif defined(CONFIG_x86_64)
#define DIRECTORY_ADDR ((directory_entry *)0xFFFFF000)
#endif

#define NUM_PAGES 1024

#define FREE 0
#define IN_USE 1

#define free_page_bitmap_size NUM_PAGES / 8
uint8_t phys_free_page_bitmap[free_page_bitmap_size] = {FREE};

#define PHYS_PAGE_START 0x200000

struct pagedir init_pgdir;
struct pagedir *current_pagedir = &init_pgdir;
void *get_free_phys_page_dir(struct pagedir dir)
{
    u64 i = 0;

    while (i < NUM_PAGES)
    {
        if (((phys_free_page_bitmap[i / 8] >> (i % 8)) & 1) == FREE)
        {
            break;
        }

        i++;
    }

    phys_free_page_bitmap[i / 8] |= IN_USE << (i % 8);

    dir.physical_pagetables[i] = (PHYS_PAGE_START + (i * PAGE_SIZE)) | 0x03;
    /* printf("test\n"); */

    void *addr = (void *)(PHYS_PAGE_START + (i * PAGE_SIZE));

#ifdef CONFIG_x86_64
    /* memset(addr + 0xffffc00000000000, 0, PAGE_SIZE); */
#endif

    return addr;
}

void *get_free_phys_page()
{
    return get_free_phys_page_dir(*current_pagedir);
}

void map_page(void *physaddr, void *virtualaddr, unsigned int flags)
{
    write_pagedir(current_pagedir, physaddr, virtualaddr, flags);
    flush_tlb_full();
}

struct pagedir *new_pagedir()
{
    struct pagedir *out = malloc(sizeof(struct pagedir));
    out->pgdir_virtual = get_free_pages(1);
    out->pgdir = get_physaddr(out->pgdir_virtual);
    out->physical_pagetables = get_free_pages(1);
    out->pagetable = get_free_pages(1);

#ifdef CONFIG_x86_32
    out->pgdir_virtual[1023] =
        (uint32_t)(out->pgdir) | PAGING_WRITEABLE | PAGING_PRESENT;
#endif

    return out;
}

void load_pgdir(struct pagedir *d)
{
    /* unsigned long cr3; */
    /* __asm__ volatile("mov %%cr3, %0" : "=r"(cr3)); */
    printf("VIRTUAL ADDRESS:%p\n", d->pgdir_virtual);
    __asm__ volatile("mov %0, %%cr3" : : "r"(d->pgdir));
    /* printf("loaded dir\n"); */

    // probably useless
    flush_tlb_full();

    current_pagedir = d;
}

void init_paging()
{
    init_gdt();

    init_pgdir =
        (struct pagedir){.pgdir = 0x1000,
                         .pgdir_virtual = DIRECTORY_ADDR,
                         .physical_pagetables = paging_page_table,
                         /* will break if this is not the current pagedir */
                         .pagetable = (void *)0xFFC00000};

    *current_pagedir = init_pgdir;

    /* test(0xD0000000, 0x1000000); */
    /* get_free_pages_test(); */
}

#define USER_STACK_BASE 0xBFFF0000
#define USER_STACK_SIZE 0x4000
uint32_t user_stack_physical = 0x300000;

void create_userspace_stack(struct pagedir *pgdir)
{
    /* printf("new userspace stack at %p\n", USER_STACK_PHYSICAL); */
    for (uint32_t i = USER_STACK_BASE - USER_STACK_SIZE; i < USER_STACK_BASE;

         i += PAGE_SIZE)
    {

        /* printf("user stack:\n"); */
        /* printf("physical:%p, virtual:%p\n", user_stack_physical, (void *)i);
         */

        /* void *page = get_free_phys_page_dir(*pgdir); */
        /* printf("%p\n", page); */

        write_pagedir(pgdir, get_free_phys_page_dir(*pgdir), (void *)i,
                      PAGING_WRITEABLE | PAGING_USER | PAGING_PRESENT);
        /* write_pagedir(pgdir, (void *)user_stack_physical, (void *)i, */
        /*               PAGING_WRITEABLE | PAGING_USER | PAGING_PRESENT); */
        /* user_stack_physical += PAGE_SIZE; */
    }
}

struct pagedir *get_current_pagedir()
{
    return current_pagedir;
}
