#include "kernel/paging.h"

void *paging_page_table_addr = 0;

void flush_tlb_full()
{
    unsigned long cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    __asm__ volatile("mov %0, %%cr3" : : "r"(cr3));
}

void unmap_page(void *virtualaddr)
{
}

void write_pagedir(struct pagedir *pgdir, void *physaddr, void *virtualaddr,
                   uint flags)
{
}

void *get_physaddr(void *virtualaddr)
{
}
