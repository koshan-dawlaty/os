#include <errno.h>
#include <kernel/addr_space.h>
#include <kernel/paging.h>
#include <kernel/utils.h>
#include <lib/assert.h>
#include <lib/list.h>
#include <lib/log.h>
#include <lib/utils.h>
#include <memory/paging_flags.h>
#include <panic.h>
#include <scheduler.h>
#include <stdlib.h>
#include <utils.h>
extern struct pagedir init_pgdir;

/* the list initalizer would not work with current_addr_space as a pointer, as
 * its value is not known at compile time, even if it is taking the address of a
 * struct */
struct addr_space current_addr_space_no_ptr = {
    &init_pgdir, LIST_HEAD_INIT(&current_addr_space_no_ptr.list_head)};
struct addr_space *current_addr_space = &current_addr_space_no_ptr;

struct addr_space *addr_space_alloc_dir(struct pagedir *dir)
{
    struct addr_space *out = malloc(sizeof(struct addr_space));
    list_init_head(&out->list_head);
    out->dir = dir;
    return out;
}

struct addr_space *addr_space_alloc()
{
    struct pagedir *dir = new_pagedir();
    write_kernel_page_tables(dir);
    struct addr_space *out = addr_space_alloc_dir(dir);
    return out;
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

void addr_space_free(struct addr_space *a)
{

    struct addr_space_page *prev = NULL;

    struct pagedir *prev_dir = get_current_pagedir();
    load_pgdir(a->dir);

    struct list *pos;
    list_for_each(pos, &a->list_head)
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

    if ((ptr_t)addr < 4096)
    {
        return -EFAULT;
    }

    irq_state_t state = irq_save_disable();
    /* logd("mapping:%p, a:%p c:%p", addr, a->dir, current_addr_space->dir); */
    /* printf("start:%p\n", a->list_head); */
    struct pagedir *prev_dir = get_current_pagedir();
    load_pgdir(a->dir);

    /* this will allocate contagious physical pages, but it is faster with a
     * bitmap allocate than individually allocating */
    void *phys = get_free_phys_pages(num_pages);

    /* printf("physaddr:%p\n", phys); */
    for (size_t i = 0; i < num_pages; i++)
    {
        write_pagedir(a->dir, phys + (i * PAGE_SIZE), addr + (i * PAGE_SIZE),
                      PAGING_PRESENT | PAGING_WRITEABLE | flags);
    }

    struct addr_space_page *page = addr_space_page_alloc(
        (void *)ptr_align((ptr_t)addr, PAGE_SIZE), num_pages);

    list_push(&page->list, &a->list_head);

    struct list *pos;
    list_for_each(pos, &a->list_head)
    {
        struct addr_space_page *pg =
            containerof(pos, struct addr_space_page, list);
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
    list_for_each(pos, &a->list_head)
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

    list_remove(pos);
    free(pg);

    load_pgdir(prev_dir);

    irq_restore(state);

    return 0;
}

int addr_space_map_current(void *addr, size_t num, int flags)
{
    /* logd("current:%p", current_addr_space); */
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
