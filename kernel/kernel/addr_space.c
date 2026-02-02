#include "kernel/utils.h"
#include <errno.h>
#include <kernel/addr_space.h>
#include <kernel/paging.h>
#include <lib/list.h>
#include <memory/paging_flags.h>
#include <stdlib.h>

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

    return page;
}

void addr_space_free(struct addr_space a)
{
    /* TODO: free pagedir */
}

int addr_space_map(struct addr_space a, void *addr, size_t num_pages, int flags)
{
    struct pagedir *prev_dir = get_current_pagedir();
    load_pgdir(a.dir);
    for (size_t i = 0; i < num_pages; i++)
    {
        write_pagedir(a.dir, get_free_phys_page(), addr + (i * PAGE_SIZE),
                      PAGING_PRESENT | flags);
    }

    struct addr_space_page *page = addr_space_page_alloc(
        (void *)ptr_align((ptr)addr, PAGE_SIZE), num_pages);

    if (a.list_head != NULL)
    {
        list_push(&page->list, &a.list_head);
    }
    else
    {
        a.list_head = &page->list;
    }

    load_pgdir(prev_dir);

    return 0;
}

int addr_space_unmap(struct addr_space a, void *addr)
{
    struct pagedir *prev_dir = get_current_pagedir();
    load_pgdir(a.dir);

    struct addr_space_page *pg = NULL;
    struct list *pos = NULL;
    list_for_each(pos, a.list_head)
    {
        pg = LIST_ENTRY(pos, struct addr_space_page, list);

        if (pg->addr == addr)
        {
            break;
        }
    }

    if (pg == NULL)
    {
        return -ENOENT;
    }

    size_t num_pages = pg->num_pages;
    for (size_t i = 0; i < num_pages; i++)
    {
        unmap_page(addr + (i * PAGE_SIZE));
    }

    free(pg);

    load_pgdir(prev_dir);

    return 0;
}
