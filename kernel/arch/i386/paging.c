#include "gdt.h"
#include "interrupts/idt.h"
#include "kernel/tty.h"
#include "kernel/utils.h"
#include "multiboot.h"
#include "vga.h"
#include <stdint.h>
#include <stdio.h>

typedef uint32_t directory_entry;
typedef uint32_t table_entry;

/* extern int paging_page_table_addr; */
/* static table_entry *paging_page_table = */
/*     (table_entry *)((uint32_t)&paging_page_table_addr); */

extern long paging_page_table_addr;
/* extern long boot_page_directory; */

/* static table_entry *paging_page_table = (table_entry
 * *)paging_page_table_addr; */
#define paging_page_table ((table_entry *)&paging_page_table_addr)
#define paging_page_table_addr_page                                            \
    ((table_entry *)&paging_page_table_addr_page_addr)

static int paging_page_table_offset = 0;
static int paging_page_table_addr_page_offset = 0;

#define PAGE_SIZE 4096
#define DIRECTORY_ADDR ((directory_entry *)0xFFFFF000)

extern uint32_t _kernel_end;
#define endkernel ((_kernel_end) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1)

/* #define endkernel _kernel_end + 0xC1000000 */

#define NUM_PAGES 1024

#define FREE 0
#define IN_USE 1

#define free_page_bitmap_size NUM_PAGES / 8
static uint8_t free_page_bitmap[free_page_bitmap_size] = {FREE};

#define PHYS_PAGE_START 0x100000

void *get_free_phys_page()
{

    int i = 0;

    while (i < NUM_PAGES)
    {
        if (((free_page_bitmap[i / 8] >> (i % 8)) & 1) == FREE)
        {
            break;
        }

        i++;
    }

    free_page_bitmap[i / 8] |= IN_USE << (i % 8);

    paging_page_table[i] = (PHYS_PAGE_START + (i * PAGE_SIZE)) | 0x03;

    /* free_page_bitmap[i] = IN_USE; */
    /* printf("i is: %d\n", i); */

    return (void *)(PHYS_PAGE_START + (i * PAGE_SIZE));
}

void *get_physaddr(void *virtualaddr)
{

    directory_entry pdindex = (directory_entry)virtualaddr >> 22;
    directory_entry ptindex = (directory_entry)virtualaddr >> 12 & 0x03FF;
    /* printf("test safasdkjfl %p\n", ptindex); */

    /* directory_entry *pd = (directory_entry *)0xFFFFF000; */
    directory_entry *pd = DIRECTORY_ADDR;
    // Here you need to check whether the PD entry is present.

    directory_entry dir_entry = pd[pdindex];

    /* printf("dir\n"); */
    /* printf("%d\n", (dir_entry & 0x1) == 0); */

    /* return (void *)0; */

    /* printf("index: %d\n", pdindex); */

    if ((dir_entry & 0x1) == 0)
    {
        /* printf("page fault (directory)\n\n"); */
        return (void *)0;
    }

    table_entry *pt = ((table_entry *)0xFFC00000) + (0x400 * pdindex);
    // Here you need to check whether the PT entry is present.

    /* printf("page table %p p \n", pt); */

    /* printf("index: %d\n\n", ptindex); */
    table_entry table_entry = pt[ptindex];

    /* printf("table %p\n", table_entry); */
    /* printf("table addr %p\n", pt); */
    /* printf("table index %x\n", ptindex); */

    /* printf("%d\n", (table_entry & 0x1) == 0); */

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

        table_entry *new_table = get_free_phys_page();

        /* table_entry *virtual_table = */
        /*     ((table_entry *)0xFFC00000) + (0x400 * pdindex); */

        pd[pdindex] = ((directory_entry)(new_table)) | 0x03;

        /* printf( */
        /*     "the table %p, table (fake) %p, indejx %d, imndexdri cectory %d\n
         * ", */
        /*     (table_entry)new_table, virtual_table, ptindex, pdindex); */

        /* printf("dir %p\n", pd[pdindex]); */

        /* paging_page_table[paging_page_table_offset] = */
        /*     (table_entry)new_table + 0x03; */

        /* paging_page_table_offset++; */

        /* table_entry new_addr = (directory_entry)physical_addr; */

        /* printf("table %p, table fake %p\n", new_table, virtual_table); */

        /* virtual_table[ptindex] = new_addr | (flags & 0xFFF) | 0x01; */

        /* printf("%p, %p\n", virtual_table, */
        /*        (virtual_table) + (ptindex * sizeof(table_entry))); */

        /* printf("help\n"); */

        /* return; */
    }

    table_entry *pt = ((table_entry *)0xFFC00000) + (0x400 * pdindex);

    // Here you need to check whether the PT entry is present.
    // When it is, then there is already a mapping present. What do you do now?

    table_entry table_entry = pt[ptindex];

    printf("mapping at dir index: %p, table index: %p\n", pdindex, ptindex);
    printf("entry: %p\n", table_entry);

    if ((table_entry & 0x1) != 0)
    {
        printf("mapping already present!\n");

        return;
    }

    printf("mapping worked\n");

    pt[ptindex] =
        ((directory_entry)physical_addr) | (flags & 0xFFF) | 0x01; // Present

    // Now you need to flush the entry in the TLB
    // or you might not notice the change.

    // slow
    flush_tlb_full();
}

void paging_test()
{
    void *virtual = (void *)0xF0000000;
    void *physical = (void *)0x10000000;

    for (int i = 0; i < 1024; i++)
    {
        /* printf("%d\n", i); */

        virtual += PAGE_SIZE;
        physical += PAGE_SIZE;

        map_page(physical, virtual, 0);

        if (get_physaddr(virtual) != physical)
        {
            printf("test failed\n");
            printf("virtual: %p, physical: %p, result: %p\n", virtual, physical,
                   get_physaddr(virtual));
            return;
        }
    }

    printf("test ok\n");
}

void test(uint32_t virtual, uint32_t physical)
{
    map_page((void *)physical, (void *)virtual, 0);

    printf("%p\n", get_physaddr((void *)virtual));
}

void init_paging()
{

    init_gdt();

    init_idt();

    /* paging_test(); */

    /* test(0x20001000, 0x10001000); */
    /* test(0x20000000, 0x10002000); */

    /* test(0xF2400000, 0x30002000); */

    /* void *virtual = (void *)0x20001000; */
    /* void *physical = (void *)0x10001000; */
    /* map_page(physical, virtual, 0); */

    /* printf("%p\n", get_physaddr(virtual)); */

    /* void *virtual2 = (void *)0x20000000; */
    /* void *physical2 = (void *)0x10002000; */
    /* map_page(physical2, virtual2, 0); */

    /* printf("%p\n", get_physaddr(virtual2)); */

    /* void *virtual3 = (void *)0x20003000; */
    /* void *physical3 = (void *)0x30001000; */
    /* map_page(physical3, virtual3, 0); */

    /* printf("%p\n", get_physaddr(virtual3)); */
}
