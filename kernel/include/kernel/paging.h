#ifndef PAGING_H
#define PAGING_H
#include "lib/log.h"
#include "memory/get_free_pages.h"
#include <memory/get_free_pages.h>
#include <types.h>
#define PAGE_SIZE 4096

#include <multiboot.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

struct pagedir
{
    ptr_t *pgdir;
    /* the virtual address of the page directory in the initial page directory
     */
    ptr_t *pgdir_virtual;
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
void free_phys_pages(void *addr, size_t num);

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
void *get_free_phys_pages(size_t amount);

/*
 * maps physical address to an arbitrary virtual address using flags
 * @num_pages number of contiguous physical pages to map to contiguous virtual
 * pages
 */
static inline void *map_physaddr(void *physaddr, size_t num_pages, uint flags)
{
    /* FIXME: this should use the binary tree allocation api */
    void *virtual = get_free_pages_unmmaped(num_pages, &gfp_kernel_zone);
    for (size_t i = 0; i < num_pages; i++)
    {
        map_page(physaddr + i * PAGE_SIZE, virtual + i * PAGE_SIZE, flags);
    }

    return virtual;
}

/* TODO: test this */
static inline void free_mapped_physaddr(void *virtual_addr, size_t num_pages)
{
    free_pages(virtual_addr, num_pages);
    void *physaddr = get_physaddr(virtual_addr);
    for (size_t i = 0; i < num_pages; i++)
    {
        unmap_page(physaddr + i * PAGE_SIZE);
    }
}

/* this might only be needed for x86 */
extern struct pagedir init_pgdir;

/* TODO: this should be a gfp zone */
static inline void *dma_alloc_pages(size_t num)
{
    void *physaddr = get_free_phys_pages(num);
    logd("got %p phys page", physaddr);
    return map_physaddr(physaddr, num, PAGING_PRESENT | PAGING_WRITEABLE);
}

static inline void dma_free_pages(void *pages, size_t num)
{
    free_mapped_physaddr(pages, num);
}

#endif
