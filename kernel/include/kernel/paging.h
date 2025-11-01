#ifndef PAGING_H
#define PAGING_H
#include "kernel/types.h"
#define PAGE_SIZE 4096

#include "multiboot.h"
#include "sys/types.h"
#include <stddef.h>
#include <stdint.h>

struct pagedir
{
    ptr *pgdir;
    /* the virtual address of the page directory in the initial page directory
     */
    ptr *pgdir_virtual;
    /* used to map physical pages */
    uint32_t *physical_pagetables;
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

void *get_free_pages(size_t amount);
void *get_free_phys_page();

void free_pages(void *addr, size_t amount);

void *get_free_pages_user(size_t amount);
void free_pages_user(void *addr, size_t amount);

/*
 * does not set the present bit in the page
 */
void write_pagedir(struct pagedir *pgdir, void *physaddr, void *virtualaddr,
                   uint flags);

/*
 * writes the page tables above 0xC0000000 to the page directory
 */
void write_kernel_page_tables(struct pagedir *pgdir);

/*
 * creates a new userspace stack and writes it to the pagedir
 */
void create_userspace_stack(struct pagedir *pgdir);

struct pagedir *get_current_pagedir();

#endif
