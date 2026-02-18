#include <errno.h>
#include <kernel/addr_space.h>
#include <kernel/paging.h>
#include <kernel/utils.h>
#include <lib/list.h>
#include <memory/paging_flags.h>
#include <panic.h>
#include <stdlib.h>
extern struct pagedir init_pgdir;

struct addr_space current_addr_space = {.dir = &init_pgdir, .list_head = NULL};

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
    list_for_each(pos, a.list_head)
    {
        struct addr_space_page *pg =
            LIST_ENTRY(pos, struct addr_space_page, list);

        for (size_t i = 0; i < pg->num_pages; i++)
        {
            unmap_page(pg->addr + (i * PAGE_SIZE));
        }

        if (prev != NULL)
        {
            free(prev);
        }
        prev = pg;
    }
}

int addr_space_map(struct addr_space *a, void *addr, size_t num_pages,
                   int flags)
{
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

    load_pgdir(prev_dir);

    /* printf("%p\n", a->list_head); */

    return 0;
}

int addr_space_unmap(struct addr_space *a, void *addr, size_t num_pages)
{
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
        return -ENOENT;
    }

    for (size_t i = 0; i < num_pages; i++)
    {
        unmap_page(addr + (i * PAGE_SIZE));
    }

    list_remove(pos, &a->list_head);
    free(pg);

    load_pgdir(prev_dir);

    return 0;
}
