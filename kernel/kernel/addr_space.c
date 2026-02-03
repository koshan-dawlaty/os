#include "lib/binary_tree.h"
#include <container_of.h>
#include <errno.h>
#include <kernel/addr_space.h>
#include <kernel/paging.h>
#include <kernel/utils.h>
#include <lib/list.h>
#include <memory/paging_flags.h>
#include <panic.h>
#include <stdlib.h>
extern struct pagedir init_pgdir;

struct addr_space current_addr_space = {.dir = &init_pgdir,
                                        .head = BINTREE_HEAD_INIT()};

struct addr_space addr_space_alloc()
{
    struct pagedir *dir = new_pagedir();
    write_kernel_page_tables(dir);
    return (struct addr_space){.dir = dir, .head = BINTREE_HEAD_INIT()};
}

static struct addr_space_page *addr_space_page_alloc(void *addr,
                                                     size_t num_pages)
{
    struct addr_space_page *page = malloc(sizeof(struct addr_space_page));

    page->addr = addr;
    page->num_pages = num_pages;
    /* TODO: remove node->value from struct bintree_node, and do what the linux
     * kernel does, make the user of the binary tree api do the operations
     * https://docs.kernel.org/core-api/rbtree.html
     * */
    page->node = BINTREE_NODE_INIT(0);

    return page;
}

void addr_space_free(struct addr_space a)
{

    /*
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
    */
}

int addr_space_map(struct addr_space *a, void *addr, size_t num_pages,
                   int flags)
{
    /* printf("start:%p\n", a->list_head); */
    /* printf("mapping:%p\n", addr); */
    struct pagedir *prev_dir = get_current_pagedir();
    load_pgdir(a->dir);
    for (size_t i = 0; i < num_pages; i++)
    {
        write_pagedir(a->dir, get_free_phys_page(), addr + (i * PAGE_SIZE),
                      PAGING_PRESENT | flags);
    }

    struct addr_space_page *page = addr_space_page_alloc(
        (void *)ptr_align((ptr)addr, PAGE_SIZE), num_pages);

    if (a->head.root == NULL)
    {
        a->head.root = &page->node;
    }

    load_pgdir(prev_dir);
    return 0;
}

int addr_space_unmap(struct addr_space *a, void *addr, size_t num_pages)
{
    struct pagedir *prev_dir = get_current_pagedir();
    load_pgdir(a->dir);

    struct bintree_node *node = a->head.root;
    struct addr_space_page *pg = NULL;

    while (node != NULL)
    {
        pg = containerof(node, struct addr_space_page, node);

        if (pg->addr < addr)
        {
            node = node->l;
        }
        else if (pg->addr > addr)
        {
            node = node->r;
        }
        else if (pg->addr == addr)
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
