#include "kernel/paging.h"
#include "bits/types/__fpos64_t.h"
#include "gdt.h"
#include "interrupts/idt.h"
#include "kernel/tty.h"
#include "kernel/utils.h"
#include "memory/paging_flags.h"

#include "multiboot.h"
#include "vga.h"

#include <stdint.h>
#include <stdio.h>

typedef uint32_t directory_entry;
typedef uint32_t table_entry;

extern long paging_page_table_addr;
#define paging_page_table ((table_entry *)&paging_page_table_addr)

#define PAGE_SIZE 4096
#define DIRECTORY_ADDR ((directory_entry *)0xFFFFF000)

#define NUM_PAGES 1024

#define FREE 0
#define IN_USE 1

#define free_page_bitmap_size NUM_PAGES / 8
static uint8_t phys_free_page_bitmap[free_page_bitmap_size] = {FREE};

#define PHYS_PAGE_START 0x100000

void *get_free_phys_page()
{

    int i = 0;

    while (i < NUM_PAGES)
    {
        if (((phys_free_page_bitmap[i / 8] >> (i % 8)) & 1) == FREE)
        {
            break;
        }

        i++;
    }

    phys_free_page_bitmap[i / 8] |= IN_USE << (i % 8);

    paging_page_table[i] = (PHYS_PAGE_START + (i * PAGE_SIZE)) | 0x03;

    return (void *)(PHYS_PAGE_START + (i * PAGE_SIZE));
}

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
        printf("page fault (table)\n\n");

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

void map_page(void *physaddr, void *virtualaddr, unsigned int flags)
{

    uint32_t virtual_addr = align_long((uint32_t)virtualaddr, PAGE_SIZE);
    uint32_t physical_addr = align_long((uint32_t)physaddr, PAGE_SIZE);

    directory_entry pdindex = (directory_entry)virtual_addr >> 22;
    directory_entry ptindex = (directory_entry)virtual_addr >> 12 & 0x03FF;

    directory_entry *pd = (directory_entry *)DIRECTORY_ADDR;

    directory_entry dir_entry = pd[pdindex];

    if ((dir_entry & 0x1) == 0)
    {
        /* printf("this was called!!!\n"); */

        table_entry *new_table = get_free_phys_page();

        pd[pdindex] = ((directory_entry)(new_table)) | 0x3;
    }

    table_entry *pt = ((table_entry *)0xFFC00000) + (0x400 * pdindex);

    // Here you need to check whether the PT entry is present.
    // When it is, then there is already a mapping present. What do you do now?

    table_entry table_entry = pt[ptindex];

    /* printf("mapping at dir index: %p, table index: %p\n", pdindex, ptindex);
     */
    /* printf("entry: %p\n", table_entry); */

    if ((table_entry & 0x1) != 0)
    {
        /* printf("mapping already present!\n"); */

        /* return; */
    }

    /* printf("mapping worked\n"); */

    pt[ptindex] =
        ((directory_entry)physical_addr) | (flags & 0xFFF) | PAGING_PRESENT;

    // Now you need to flush the entry in the TLB
    // or you might not notice the change.

    // slow
    flush_tlb_full();
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

void get_free_pages_test()
{
    printf("get free pages test\n");
    char *c = get_free_pages(2);
    *c = 'a';
    free_pages(c, 2);

    void *ten = get_free_pages(10);
    void *three = get_free_pages(3);
    free_pages(ten, 10);
    void *five = get_free_pages(5);

    void *one = get_free_pages(1);

    free_pages(three, 3);

    free_pages(five, 5);

    free_pages(one, 1);

    int amount = 2;
    char *pages = get_free_pages(amount);
    *pages = 'a';
    free_pages(pages, amount);

    printf("should fault after this\n");

    *pages = 'a';
}

void init_paging()
{
    init_gdt();
    init_idt();

    /* test(0xD0000000, 0x1000000); */
    /* get_free_pages_test(); */
}
