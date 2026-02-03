#ifndef ADDRESS_SPACE_H
#define ADDRESS_SPACE_H

#include "lib/binary_tree.h"
#include <kernel/paging.h>
#include <lib/list.h>

struct addr_space_page
{
    void *addr;
    size_t num_pages;
    struct bintree_node node;
};

/* wrapper for struct pagedir that keeps track of pages */
struct addr_space
{
    struct pagedir *dir;
    struct bintree_head head;
};

struct addr_space addr_space_alloc();
void addr_space_free(struct addr_space a);

extern struct addr_space current_addr_space;

static inline void addr_space_load(struct addr_space a)
{
    current_addr_space = a;
    load_pgdir(a.dir);
}

int addr_space_map(struct addr_space *a, void *addr, size_t num_pages,
                   int flags);
int addr_space_unmap(struct addr_space *a, void *addr, size_t num_pages);

#endif
