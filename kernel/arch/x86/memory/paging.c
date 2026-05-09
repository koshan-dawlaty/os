#include "memory/paging_flags.h"
#include "memory/paging_internal.h"
#include <gdt.h>
#include <interrupts/idt.h>
#include <kernel/addr_space.h>
#include <kernel/multiboot.h>
#include <kernel/paging.h>
#include <kernel/tty.h>
#include <kernel/utils.h>
#include <lib/assert.h>
#include <memory/get_free_pages.h>

#include "multiboot.h"
#include "scheduler.h"
#include "spinlock.h"
#include "utils.h"
#include "vfs.h"
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

/* #define PAGE_SIZE 4096 */

#ifdef CONFIG_x86_64
#define DIRECTORY_ADDR ((directory_entry *)0xffffc00000001000)
#elif defined(CONFIG_x86_32)
#define DIRECTORY_ADDR ((directory_entry *)0xFFFFF000)
#endif

/* TODO: write a good physical page allocator instead of relying on increasing
 * this limit, it is stupid */
/* #define NUM_PAGES 4096 */
#define NUM_PAGES 8000

#define FREE 0
#define IN_USE 1

#define free_page_bitmap_size NUM_PAGES / 8
u8 phys_free_page_bitmap[free_page_bitmap_size] = {FREE};

void *phys_page_start = (void *)0x200000;

struct spinlock phys_page_lock = SPINLOCK_INIT();

struct pagedir init_pgdir;
struct pagedir *current_pagedir = &init_pgdir;
void *get_free_phys_page_dir(struct pagedir dir)
{
    spinlock_lock(&phys_page_lock);
    ptr i = 0;

    while (i < NUM_PAGES)
    {
        if (((phys_free_page_bitmap[i / 8] >> (i % 8)) & 1) == FREE)
        {
            break;
        }

        i++;
    }

    if (i >= NUM_PAGES - 3)
    {
        PANIC("ran out of space in get_free_phys_pages");
    }

    phys_free_page_bitmap[i / 8] |= IN_USE << (i % 8);

    void *addr = (void *)(phys_page_start + (i * PAGE_SIZE));

#if defined(CONFIG_x86_64) && defined(CONFIG_CLEAR_PAGES)
    memset(addr + 0xffffc00000000000, 'b', PAGE_SIZE);
#endif

    spinlock_unlock(&phys_page_lock);

    return addr;
}

void *get_free_phys_pages(struct pagedir *dir, size_t amount)
{
    spinlock_lock(&phys_page_lock);
    ptr i;
    size_t consecutive = 0;
    size_t consecutive_start = 0;
    for (i = 0; i < NUM_PAGES; i++)
    {
        if (((phys_free_page_bitmap[i / 8] >> (i % 8)) & 1) == GFP_FREE)
        {
            /* printf("i is:%d\n", i); */
            if (consecutive == 0)
            {
                consecutive_start = i;
            }

            consecutive++;
        }
        else
        {
            consecutive_start = 0;
            consecutive = 0;
        }

        if (consecutive >= amount)
        {
            break;
        }
    }

    /* printf("start:%d\n", consecutive_start); */

    for (size_t j = consecutive_start; j < amount + i; j++)
    {
        /* printf("j is:%d\n", j); */

        /* printf("virtual:%p\nphysical:%p\n", virtual_addr, physical_addr); */
        phys_free_page_bitmap[j / 8] |= (1 << (j % 8));
    }

    void *address = (void *)((i * PAGE_SIZE) + phys_page_start);

    /* release_semaphore(get_free_pages_semaphore); */
    spinlock_unlock(&phys_page_lock);

    return address;
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
    bzero(out->pgdir_virtual, PAGE_SIZE);
    out->pgdir = get_physaddr(out->pgdir_virtual);
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
    /* printf("VIRTUAL ADDRESS:%p\n", d->pgdir_virtual); */

    __asm__ volatile("mov %0, %%cr3" : : "r"(d->pgdir));

    // probably useless
    flush_tlb_full();

    current_pagedir = d;
}

void init_paging()
{

#ifdef CONFIG_x86_32
    void *directory_physical = get_physaddr(DIRECTORY_ADDR);
#else

    void *directory_physical = (void *)0x1000;
#endif

    init_pgdir =
        (struct pagedir){.pgdir = directory_physical,
                         .pgdir_virtual = (void *)DIRECTORY_ADDR,
                         /* .physical_pagetables = paging_page_table, */
                         /* will break if this is not the current pagedir */
                         .pagetable = (void *)0xFFC00000};

    *current_pagedir = init_pgdir;

    /* test(0xD0000000, 0x1000000); */
    /* get_free_pages_test(); */

    /* assume there is at least one */
    struct memory_region max = memory_regions_get(0);
    for (size_t i = 0; i < memory_regions_len(); i++)
    {
        struct memory_region r = memory_regions_get(i);

        if (r.len > max.len)
        {
            max = r;
        }
    }
    /* TODO: this could theoretically overflow (maybe) */
    phys_page_start =
        (void *)((((ptr)(max.start) / PAGE_SIZE) + 1) * PAGE_SIZE);
}

#define USER_STACK_BASE 0xBFFF0000
#define USER_STACK_SIZE 0x4000
uint32_t user_stack_physical = 0x300000;

void create_userspace_stack(struct addr_space *a)
{
    /* printf("new userspace stack at %p\n", USER_STACK_PHYSICAL); */
    for (ptr i = USER_STACK_BASE - USER_STACK_SIZE; i < USER_STACK_BASE;

         i += PAGE_SIZE)
    {

        /* printf("user stack:\n"); */
        /* printf("physical:%p, virtual:%p\n", user_stack_physical, (void
         * *)i);
         */

        /* void *page = get_free_phys_page_dir(*pgdir); */
        /* printf("%p\n", page); */

        /* write_pagedir(pgdir, get_free_phys_page_dir(*pgdir), (void *)i,
         */
        /*               PAGING_WRITEABLE | PAGING_USER | PAGING_PRESENT);
         */

        addr_space_map(a, (void *)i, 1,
                       PAGING_PRESENT | PAGING_USER | PAGING_WRITEABLE);
        /* write_pagedir(pgdir, (void *)user_stack_physical, (void *)i, */
        /*               PAGING_WRITEABLE | PAGING_USER | PAGING_PRESENT);
         */
        /* user_stack_physical += PAGE_SIZE; */
    }
}

struct pagedir *get_current_pagedir()
{
    return current_pagedir;
}
