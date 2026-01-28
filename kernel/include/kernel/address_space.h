#ifndef ADDRESS_SPACE_H
#define ADDRESS_SPACE_H

#include <kernel/paging.h>
#include <lib/list.h>

struct addr_space_page {
    void * addr;
    struct list list;
};

/* wrapper for struct pagedir that keeps track of pages */
struct addr_space {
    struct pagedir* dir;
    struct list *list_head;
};

struct address_space address_space_alloc();

#endif