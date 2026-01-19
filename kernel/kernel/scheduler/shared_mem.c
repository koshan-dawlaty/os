#include "memory/paging_flags.h"
#include <kernel/paging.h>
#include <kernel/syscall.h>
#include <syscall.h>

/* TODO: make this dynamic */
#define SHARED_MEMORY_MAX 100
void *shared_memory_list[SHARED_MEMORY_MAX];
size_t shared_memory_list_index = 0;

#define SHARED_MEMORY_START (void *)0x40000000;

int shared_mem_alloc(void **addr)
{
    void *memory = (void *)0x40000000;
    shared_memory_list[shared_memory_list_index] = memory;
    shared_memory_list_index++;

    *addr = memory;

    return shared_memory_list_index - 1;
}

int shared_mem_get(int id, void **p)
{

    void *unmapped = shared_memory_list[id];
    map_page(get_free_phys_page(), unmapped, PAGING_PRESENT | PAGING_USER);

    *p = unmapped;

    return 0;
}

SYSCALL_DEFINE1(shared_mem_alloc, SYS_shared_mem_alloc)
SYSCALL_DEFINE2(shared_mem_get, SYS_shared_mem_get)
