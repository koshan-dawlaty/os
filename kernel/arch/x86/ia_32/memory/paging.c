#include <gdt.h>
#include <kernel/paging.h>

#include <interrupts/idt.h>
#include <kernel/tty.h>
#include <kernel/utils.h>
#include <memory/paging_flags.h>

#include <multiboot.h>
#include <vga.h>

#include <kernel/paging.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef ptr directory_entry;
typedef ptr table_entry;

extern long paging_page_table_addr;
#define paging_page_table ((table_entry *)&paging_page_table_addr)

#define PAGE_SIZE 4096
#define DIRECTORY_ADDR ((directory_entry *)0xFFFFF000)

extern struct pagedir init_pgdir;

void *get_physaddr(void *virtualaddr)
{

    directory_entry pdindex = (directory_entry)virtualaddr >> 22;
    directory_entry ptindex = (directory_entry)virtualaddr >> 12 & 0x03FF;

    directory_entry *pd = DIRECTORY_ADDR;
    // Here you need to check whether the PD entry is present.

    directory_entry dir_entry = pd[pdindex];

    if ((dir_entry & 0x1) == 0)
    {
        return (void *)0;
    }

    table_entry *pt = ((table_entry *)0xFFC00000) + (0x400 * pdindex);
    // Here you need to check whether the PT entry is present.

    table_entry table_entry = pt[ptindex];

    if ((table_entry & (0x1)) == 0)
    {
        printf("page fault (table)\n");

        return (void *)0;
    }

    return (void *)((table_entry & ~0xfff) +
                    ((directory_entry)virtualaddr & 0xfff));
}

void flush_tlb_full()
{
    unsigned long cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    __asm__ volatile("mov %0, %%cr3" : : "r"(cr3));
}

void unmap_page(void *virtual_address)
{
    uint32_t virtual_addr = align_long((uint32_t)virtual_address, PAGE_SIZE);

    directory_entry pdindex = (directory_entry)virtual_addr >> 22;
    directory_entry ptindex = (directory_entry)virtual_addr >> 12 & 0x03FF;

    directory_entry *pd = (directory_entry *)DIRECTORY_ADDR;
    table_entry *pt = ((table_entry *)0xFFC00000) + (0x400 * pdindex);

    if ((pd[pdindex] & 0x1) == 0 || (pt[ptindex] & 0x1) == 0)
    {
        return;
    }

    pt[ptindex] = 0;

    flush_tlb_full();
}

void write_pagedir(struct pagedir *pgdir, void *physaddr, void *virtualaddr,
                   uint flags)
{
    uint32_t virtual_addr = align_long((uint32_t)virtualaddr, PAGE_SIZE);
    uint32_t physical_addr = align_long((uint32_t)physaddr, PAGE_SIZE);
    /* printf("addr now:%p\n", virtual_addr); */

    directory_entry pdindex = (directory_entry)virtual_addr >> 22;
    directory_entry ptindex = (directory_entry)virtual_addr >> 12 & 0x03FF;
    /* printf("index:%d\n", pdindex); */

    directory_entry *pd = DIRECTORY_ADDR;

    if (pgdir != &init_pgdir)
    {
        /* printf("this\n"); */
    }

    directory_entry dir_entry = pd[pdindex];

    if ((dir_entry & 0x1) == 0)
    {
        if (pgdir != &init_pgdir)
        {
            /* printf("this\n"); */
        }

        table_entry *new_table = get_free_phys_page();
        if (pgdir != &init_pgdir)
        {
            /* printf("physaddr:%p, virtual:%p\n", physaddr, virtualaddr); */
            /* printf("new_table:%p, index:%p\n", new_table, pdindex); */
            /* printf("line 2:%p\n", init_pgdir.physical_pagetables); */
        }

        pd[pdindex] = ((directory_entry)(new_table)) | PAGING_PRESENT |
                      (flags & 0xFFF) | PAGING_WRITEABLE |
                      /* TODO: insecure */
                      PAGING_USER;
    }

    table_entry *pt = ((table_entry *)0xFFC00000) + (0x400 * pdindex);
    /* table_entry *pt = pgdir->pagetable + (0x400 * pdindex); */

    table_entry table_entry = pt[ptindex];

    /* printf("mapping at dir index: %p, table index: %p\n", pdindex,
     * ptindex);
     */

    if ((table_entry & 0x1) != 0)
    {
        /* printf("mapping already present\n"); */
        /* return; */
    }

    /* printf("mapping worked\n"); */
    if (pgdir != &init_pgdir)
    {
        /* printf("test %p\n", pt); */
    }

    pt[ptindex] =
        ((directory_entry)physical_addr) | (flags & 0xFFF) | PAGING_PRESENT;
    flush_tlb_full();
}

void write_kernel_page_tables(struct pagedir *pgdir)
{
    directory_entry *kernel_dir = (directory_entry *)DIRECTORY_ADDR;

    directory_entry *other_dir = (directory_entry *)pgdir->pgdir_virtual;
    /* printf("other dir:%p\n", other_dir); */
    /* printf("kernel dir:%p\n", kernel_dir); */

    for (uint32_t i = 0xC0000000; i < 0xFFC00000; i += 4096 * 1024)
    {

        uint32_t index = i >> 22;

        /* printf("%d\n", index); */
        directory_entry direntry = kernel_dir[index];
        if (!((direntry & 1) == 0))
        {
            /* printf("i:%p, index:%p\n", i, index); */
            /* printf("kernel:%p other:%p\n", kernel_dir[index],
             * other_dir[index]); */

            /* printf("i:%p, index:%d\n", i, index); */
            /* printf("i:%p, index:%d\n", i, i >> 22); */
            /* printf("kernel dir: %p\n", i); */

            other_dir[index] = kernel_dir[index];

            /* printf("kernel after:  %p\n", kernel_dir[index]); */
            /* other_dir[index] = 0x12345; */
        }
    }

    /* other_dir[1023] = */
    /*     (uint32_t)get_physaddr(other_dir) | PAGING_WRITEABLE |
     * PAGING_PRESENT; */

    /* printf("end\n"); */
}
