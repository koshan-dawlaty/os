#ifndef PAGING_H
#define PAGING_H
#define PAGE_SIZE 4096

#include "multiboot.h"
#include <stddef.h>
#include <stdint.h>

void init_paging();

void *get_physaddr(void *virtualaddr);

// this will always set the present bit
void map_page(void *physaddr, void *virtualaddr, unsigned int flags);
void unmap_page(void *virtualaddr);

void *get_free_pages(size_t amount);
void free_pages(void *addr, size_t amount);

/*
 * this will not map the pages (gets called by map_page)
 */
void *get_free_phys_pages(size_t amount);

void free_phys_pages(void *addr, size_t amount);

#endif
