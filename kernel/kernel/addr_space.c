#include "lib/assert.h"
#include <errno.h>
#include <kernel/addr_space.h>
#include <kernel/paging.h>
#include <kernel/utils.h>
#include <lib/list.h>
#include <lib/log.h>
#include <lib/utils.h>
#include <memory/paging_flags.h>
#include <panic.h>
#include <scheduler.h>
#include <stdlib.h>
#include <utils.h>
extern struct pagedir init_pgdir;

struct addr_space *current_addr_space =
    &(struct addr_space){.dir = &init_pgdir, .list_head = NULL};

struct addr_space addr_space_alloc()
{
    struct pagedir *dir = new_pagedir();
    write_kernel_page_tables(dir);
    return (struct addr_space){.dir = dir, .list_head = NULL};
}

static struct addr_space_page *addr_space_page_alloc(void *addr,
                                                     size_t num_pages)
{
    struct addr_space_page *page = malloc(sizeof(struct addr_space_page));

    page->addr = addr;
    page->num_pages = num_pages;
    page->list = LIST_INIT();

    return page;
}

void addr_space_free(struct addr_space a)
{

    struct list *pos;
    struct addr_space_page *prev = NULL;

    struct pagedir *prev_dir = get_current_pagedir();
    load_pgdir(a.dir);
    list_for_each(pos, a.list_head)
    {
        struct addr_space_page *pg =
            LIST_ENTRY(pos, struct addr_space_page, list);

        for (size_t i = 0; i < pg->num_pages; i++)
        {
            void *virtual = pg->addr + (i * PAGE_SIZE);

            void *physaddr = get_physaddr(virtual);
            /* TODO: for whatever reason pages mapped by the elf loader will
             * sometimes be null when getting the address from the pagetable */
            if (physaddr != NULL)
            {
                free_phys_pages(physaddr, 1);
                unmap_page(virtual);
            }

            /*
NOT_NULL(physaddr,
         "get_physaddr() returned an invalid address %p, page %p "
         "might not have been mapped",
         physaddr, virtual);
                             */
        }

        if (prev != NULL)
        {
            free(prev);
        }
        prev = pg;
    }
    load_pgdir(prev_dir);
}

int addr_space_map(struct addr_space *a, void *addr, size_t num_pages,
                   int flags)
{

    irq_state_t state = irq_save_disable();
    /* logd("mapping:%p, a:%p c:%p", addr, a->dir, current_addr_space->dir); */
    /* printf("start:%p\n", a->list_head); */
    struct pagedir *prev_dir = get_current_pagedir();
    load_pgdir(a->dir);
    for (size_t i = 0; i < num_pages; i++)
    {
        void *phys = get_free_phys_page();

        /* printf("physaddr:%p\n", phys); */
        write_pagedir(a->dir, phys, addr + (i * PAGE_SIZE),
                      PAGING_PRESENT | PAGING_WRITEABLE | flags);
    }

    struct addr_space_page *page = addr_space_page_alloc(
        (void *)ptr_align((ptr)addr, PAGE_SIZE), num_pages);

    if (a->list_head != NULL)
    {
        list_push(&page->list, &a->list_head);
    }
    else
    {
        a->list_head = &page->list;
    }

    struct list *pos;
    list_for_each(pos, a->list_head)
    {
        struct addr_space_page *pg =
            containerof(pos, struct addr_space_page, list);
        /* logd("%p, %d, %p", pg->addr, pg->num_pages, a->dir); */
    }

    load_pgdir(prev_dir);

    irq_restore(state);
    /* printf("%p\n", a->list_head); */

    return 0;
}

int addr_space_unmap(struct addr_space *a, void *addr, size_t num_pages)
{

    irq_state_t state = irq_save_disable();

    struct pagedir *prev_dir = get_current_pagedir();

    load_pgdir(a->dir);

    struct addr_space_page *pg = NULL;
    struct list *pos = NULL;
    list_for_each(pos, a->list_head)
    {
        pg = LIST_ENTRY(pos, struct addr_space_page, list);

        if (pg->addr == addr)
        {
            break;
        }
    }

    if (pg == NULL)
    {
        irq_restore(state);
        return -ENOENT;
    }

    for (size_t i = 0; i < num_pages; i++)
    {
        free_phys_pages(get_physaddr(addr + (i * PAGE_SIZE)), 1);
        unmap_page(addr + (i * PAGE_SIZE));
    }

    list_remove(pos, &a->list_head);
    free(pg);

    load_pgdir(prev_dir);

    irq_restore(state);

    return 0;
}

int addr_space_map_current(void *addr, size_t num, int flags)
{
    /* struct addr_space a = current_addr_space; */
    /* struct task *current = get_current_task(); */
    /* if (current != NULL) */
    /* { */
    /*     a = current->addr_space; */
    /* } */

    return addr_space_map(current_addr_space, addr, num, flags);
}

void addr_space_load(struct addr_space *a)
{
    current_addr_space = a;
    /* struct task *current = get_current_task(); */
    /* if (current != NULL) */
    /* { */
    /*     current->addr_space = a; */
    /* } */
    load_pgdir(a->dir);
}

struct addr_space *get_current_addr_space()
{
    return current_addr_space;
}
