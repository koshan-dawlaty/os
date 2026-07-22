#include <kernel/paging.h>
#include <kernel/utils.h>
#include <lib/log.h>
#include <spinlock.h>
#include <string.h>
#include <types.h>

void *paging_page_table_addr = 0;

void flush_tlb_full()
{
    u64 cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    __asm__ volatile("mov %0, %%cr3" : : "r"(cr3));
}

void write_kernel_page_tables(struct pagedir *pgdir)
{

    for (u64 i = 384; i < 512; i++)

    {

        u64 direntry = init_pgdir.pgdir_virtual[i];
        if (!((direntry & 1) == 0))
        {
            pgdir->pgdir_virtual[i] = init_pgdir.pgdir_virtual[i];
        }
    }

    flush_tlb_full();
}

static inline void *get_virtual_addr(void *physical_addr)
{
    return physical_addr + 0xffffc00000000000;
}

struct spinlock paging_spinlock = SPINLOCK_INIT();

void unmap_page(void *virtualaddr)
{

    spinlock_lock(&paging_spinlock);

    u64 virtual = align_long((u64)virtualaddr, PAGE_SIZE);

    u64 pml4_index = (virtual >> 39) & 0x1FF;
    u64 pdpt_index = (virtual >> 30) & 0x1FF;
    u64 pdt_index = (virtual >> 21) & 0x1FF;
    u64 pt_index = (virtual >> 12) & 0x1FF;

    /* TODO: bad */
    u64 *pml4 = (void *)get_current_pagedir();
    u64 pml4_entry = pml4[pml4_index];

    if ((pml4_entry & 1) == 0)
    {
        /* pml4[pml4_index] = (u64)get_free_phys_page() | 0x3 | flags; */
    }
    else
    {
        spinlock_unlock(&paging_spinlock);
        return;
    }

    /* TODO: math might be wrong */
    u64 *pdpt_physical = (void *)(((pml4[pml4_index]) & ~0xFFF));
    // u64 *pdpt_physical = (void *)(pml4[pml4_index]);
    u64 *pdpt = get_virtual_addr(pdpt_physical);
    u64 pdpt_entry = pdpt[pdpt_index];

    if ((pdpt_entry & 1) == 0)
    {
        /* pdpt[pdpt_index] = (u64)get_free_phys_page() | 0x3 | flags; */
    }
    else
    {
        spinlock_unlock(&paging_spinlock);
        return;
    }

    u64 *pdt_physical = (void *)((pdpt[pdpt_index]) & ~0xFFF);
    u64 *pdt = get_virtual_addr(pdt_physical);
    u64 pdt_entry = pdt[pdt_index];

    if ((pdt_entry & 1) == 0)
    {
        /* pdt[pdt_index] = (u64)get_free_phys_page() | 0x3 | flags; */
    }
    else
    {
        return;
    }

    u64 *pt_physical = (void *)((pdt[pdt_index]) & ~0xFFF);
    u64 *pt = get_virtual_addr(pt_physical);
    pt[pt_index] = 0;
    spinlock_unlock(&paging_spinlock);

    flush_tlb_full();
}

void write_pagedir(struct pagedir *pgdir, void *physaddr, void *virtualaddr,
                   uint flags)
{
    spinlock_lock(&paging_spinlock);

    u64 virtual = align_long((u64)virtualaddr, PAGE_SIZE);
    u64 physical = align_long((u64)physaddr, PAGE_SIZE);

    u64 pml4_index = (virtual >> 39) & 0x1FF;
    u64 pdpt_index = (virtual >> 30) & 0x1FF;
    u64 pdt_index = (virtual >> 21) & 0x1FF;
    u64 pt_index = (virtual >> 12) & 0x1FF;

    /* TODO: bad */
    u64 *pml4 = (void *)pgdir->pgdir_virtual;
    u64 pml4_entry = pml4[pml4_index];

    if ((pml4_entry & 1) == 0)
    {
        void *page = get_free_phys_page();

        void *pg_virtual = get_virtual_addr(page);
        bzero(pg_virtual, PAGE_SIZE);

        pml4[pml4_index] = (u64)page | 0x3 | flags;
    }

    /* TODO: math might be wrong */
    u64 *pdpt_physical = (void *)(((pml4[pml4_index]) & ~0xFFF));
    // u64 *pdpt_physical = (void *)(pml4[pml4_index]);
    u64 *pdpt = get_virtual_addr(pdpt_physical);
    u64 pdpt_entry = pdpt[pdpt_index];

    if ((pdpt_entry & 1) == 0)
    {
        void *page = get_free_phys_page();
        void *pg_virtual = get_virtual_addr(page);
        bzero(pg_virtual, PAGE_SIZE);

        pdpt[pdpt_index] = (u64)page | 0x3 | flags;
    }

    u64 *pdt_physical = (void *)((pdpt[pdpt_index]) & ~0xFFF);
    u64 *pdt = get_virtual_addr(pdt_physical);
    u64 pdt_entry = pdt[pdt_index];

    if ((pdt_entry & 1) == 0)
    {
        void *page = get_free_phys_page();
        void *pg_virtual = get_virtual_addr(page);
        bzero(pg_virtual, PAGE_SIZE);
        pdt[pdt_index] = (u64)page | 0x3 | flags;
    }

    u64 *pt_physical = (void *)((pdt[pdt_index]) & ~0xFFF);
    u64 *pt = get_virtual_addr(pt_physical);
    pt[pt_index] = physical | 0x3 | flags;
    spinlock_unlock(&paging_spinlock);

    flush_tlb_full();
}

void *get_physaddr(void *virtualaddr)
{

    u64 virtual = align_long((u64)virtualaddr, PAGE_SIZE);

    u64 pml4_index = (virtual >> 39) & 0x1FF;
    u64 pdpt_index = (virtual >> 30) & 0x1FF;
    u64 pdt_index = (virtual >> 21) & 0x1FF;
    u64 pt_index = (virtual >> 12) & 0x1FF;

    u64 *pml4 = (void *)get_current_pagedir()->pgdir_virtual;
    u64 pml4_entry = pml4[pml4_index];

    if ((pml4_entry & 1) == 0)
    {
        return (void *)0;
    }

    u64 *pdpt_physical = (void *)(((pml4[pml4_index]) & ~0xFFF));
    // u64 *pdpt_physical = (void *)(pml4[pml4_index]);
    u64 *pdpt = get_virtual_addr(pdpt_physical);
    u64 pdpt_entry = pdpt[pdpt_index];

    if ((pdpt_entry & 1) == 0)
    {
        return (void *)0;
    }

    u64 *pdt_physical = (void *)((pdpt[pdpt_index]) & ~0xFFF);
    u64 *pdt = get_virtual_addr(pdt_physical);
    u64 pdt_entry = pdt[pdt_index];

    if ((pdt_entry & 1) == 0)
    {
        return (void *)0;
    }

    u64 *pt_physical = (void *)((pdt[pdt_index]) & ~0xFFF);
    u64 *pt = get_virtual_addr(pt_physical);

    u64 pt_entry = pt[pt_index];

    if (pt_entry == 0)
    {
        /* if they passed a virtual address that was like 0xFFFFF123 we would
         * return 0, instead of 123 */
        return NULL;
    }

    flush_tlb_full();
    return (void *)((pt_entry & ~0xfff) + ((u64)virtualaddr & 0xfff));
}
