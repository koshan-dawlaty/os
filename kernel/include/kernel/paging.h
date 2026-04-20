#ifndef PAGING_H
#define PAGING_H
#include "memory/get_free_pages.h"
#include <kernel/types.h>
#define PAGE_SIZE 4096

#include <multiboot.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

struct pagedir
{
    ptr *pgdir;
    /* the virtual address of the page directory in the initial page directory
     */
    ptr *pgdir_virtual;
    /* virtual address of the self referencing page table */
    uint32_t *pagetable;
};

void load_pgdir(struct pagedir *d);

struct pagedir *new_pagedir();

void init_paging();

void *get_physaddr(void *virtualaddr);

// this will always set the present bit
void map_page(void *physaddr, void *virtualaddr, unsigned int flags);
void unmap_page(void *virtualaddr);

void *get_free_phys_page();

/*
 * does not set the present bit in the page
 */
void write_pagedir(struct pagedir *pgdir, void *physaddr, void *virtualaddr,
                   uint flags);

/*
 * writes the page tables above 0xC0000000 to the page directory
 */
void write_kernel_page_tables(struct pagedir *pgdir);

struct pagedir *get_current_pagedir();
void *get_free_phys_pages(struct pagedir *dir, size_t amount);

/*
 * maps physical address to an arbitrary virtual address using flags
 * @num_pages number of contiguous physical pages to map to contiguous virtual
 * pages
 */
static inline void *map_physaddr(void *physaddr, size_t num_pages, uint flags)
{
    /* FIXME: this should use the binary tree allocation api */
    void *virtual = get_free_pages_unmmaped(num_pages, &gfp_kernel_zone);
    map_page(physaddr, virtual, flags);

    return virtual;
}

#endif
