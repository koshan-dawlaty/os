#include "kernel/paging.h"
#include "memory/paging_flags.h"
#include <kernel/addr_space.h>

struct addr_space addr_space_alloc()
{
    return (struct addr_space){.dir = new_pagedir(), .list_head = NULL};
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

    load_pgdir(prev_dir);

    return 0;
}

int addr_space_unmap(struct addr_space a, void *addr)
{
    struct pagedir *prev_dir = get_current_pagedir();
    load_pgdir(a.dir);

    unmap_page(addr);

    load_pgdir(prev_dir);

    return 0;
}
