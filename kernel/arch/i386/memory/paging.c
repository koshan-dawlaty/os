#include "kernel/paging.h"
#include "gdt.h"
#include "interrupts/idt.h"
#include "kernel/tty.h"
#include "kernel/utils.h"
#include "memory/paging_flags.h"

#include "multiboot.h"
#include "vga.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

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

struct pagedir init_pgdir;
void *get_free_phys_page_dir(struct pagedir dir)
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

    dir.physical_pagetables[i] = (PHYS_PAGE_START + (i * PAGE_SIZE)) | 0x03;
    /* printf("test\n"); */

    return (void *)(PHYS_PAGE_START + (i * PAGE_SIZE));
}

void *get_free_phys_page()
{
    return get_free_phys_page_dir(init_pgdir);
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
    write_pagedir(&init_pgdir, physaddr, virtualaddr, flags);
    flush_tlb_full();
    return;
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

        table_entry *new_table = get_free_phys_page_dir(*pgdir);
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
}

void write_kernel_page_tables(struct pagedir *pgdir)
{
    directory_entry *kernel_dir = (directory_entry *)DIRECTORY_ADDR;

    printf("%p\n", pgdir->pgdir_virtual);
    directory_entry *other_dir = (directory_entry *)pgdir->pgdir_virtual;

    for (uint32_t i = 0xC0000000; i < 0xFFC00000; i += 4096 * 1024)
    {

        uint32_t index = i >> 22;

        /* printf("%d\n", index); */
        directory_entry direntry = kernel_dir[index];
        if (!((direntry & 1) == 0))
        {
            printf("i:%p, index:%p\n", i, index);
            /* printf("kernel:%p other:%p\n", kernel_dir, other_dir); */
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

struct pagedir *new_pagedir()
{
    struct pagedir *out = malloc(sizeof(struct pagedir));
    out->pgdir_virtual = get_free_pages(1);
    out->pgdir = get_physaddr(out->pgdir_virtual);
    out->physical_pagetables = get_free_pages(1);
    out->pagetable = get_free_pages(1);

    out->pgdir_virtual[1023] =
        (uint32_t)(out->pgdir) | PAGING_WRITEABLE | PAGING_PRESENT;

    return out;
}

void load_pgdir(struct pagedir *d)
{
    /* unsigned long cr3; */
    /* __asm__ volatile("mov %%cr3, %0" : "=r"(cr3)); */
    /* printf("trying to load dir\n"); */
    __asm__ volatile("mov %0, %%cr3" : : "r"(d->pgdir));
    /* printf("loaded dir\n"); */

    // probably useless
    flush_tlb_full();
}

void init_paging()
{
    init_gdt();
    init_idt();

    init_pgdir =
        (struct pagedir){.pgdir = get_physaddr(DIRECTORY_ADDR),
                         .pgdir_virtual = DIRECTORY_ADDR,
                         .physical_pagetables = paging_page_table,
                         /* will break if this is not the current pagedir */
                         .pagetable = (void *)0xFFC00000};

    /* test(0xD0000000, 0x1000000); */
    /* get_free_pages_test(); */
}

#define USER_STACK_BASE 0xBFFF0000
#define USER_STACK_SIZE 0x4000
uint32_t USER_STACK_PHYSICAL = 0x3000000;

void create_userspace_stack(struct pagedir *pgdir)
{
    printf("new userspace stack at %p\n", USER_STACK_PHYSICAL);
    for (uint32_t i = USER_STACK_BASE - USER_STACK_SIZE; i < USER_STACK_BASE;

         i += PAGE_SIZE)
    {

        /* printf("physical:%p, virtual:%p\n", USER_STACK_PHYSICAL, (void *)i);
         */

        write_pagedir(pgdir, get_free_phys_page_dir(*pgdir), (void *)i,
                      PAGING_WRITEABLE | PAGING_USER | PAGING_PRESENT);
        /* write_pagedir(pgdir, (void *)USER_STACK_PHYSICAL, (void *)i, */
        /*               PAGING_WRITEABLE | PAGING_USER | PAGING_PRESENT); */
        USER_STACK_PHYSICAL += PAGE_SIZE;
    }
}
