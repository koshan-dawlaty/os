#ifndef ADDRESS_SPACE_H
#define ADDRESS_SPACE_H

#include <kernel/paging.h>
#include <lib/list.h>

struct addr_space_page
{
    void *addr;
    size_t num_pages;
    struct list list;
};

/* wrapper for struct pagedir that keeps track of pages */
struct addr_space
{
    struct pagedir *dir;
    struct list *list_head;
};

struct addr_space addr_space_alloc();
void addr_space_free(struct addr_space a);

extern struct addr_space current_addr_space;

void addr_space_load(struct addr_space a);

int addr_space_map(struct addr_space *a, void *addr, size_t num_pages,
                   int flags);
int addr_space_unmap(struct addr_space *a, void *addr, size_t num_pages);

/*
 * creates a new userspace stack and writes it to the addr_space
 */
void create_userspace_stack(struct addr_space *a);

#endif
