#include "kernel/utils.h"
#include "memory/get_free_pages.h"
#include "memory/paging_flags.h"
#include <kernel/paging.h>
#include <kernel/syscall.h>
#include <syscall.h>

struct shared_memory_entry
{
    void *mem;
    size_t len;
};

/* TODO: make this dynamic */
#define SHARED_MEMORY_MAX 100
struct shared_memory_entry shared_memory_list[SHARED_MEMORY_MAX];
size_t shared_memory_list_index = 0;

#define SHARED_MEMORY_START (void *)0x40000000
size_t shared_memory_offset = 0;

int shared_mem_alloc(void **addr, size_t len)
{

    void *memory = SHARED_MEMORY_START + shared_memory_offset;
    size_t mem_len = align_long(len, PAGE_SIZE);
    size_t num_pages = (mem_len / PAGE_SIZE) + 1;
    void *physical = get_free_phys_pages(get_current_pagedir(), num_pages);
    for (size_t i = 0; i < num_pages; i++)
    {
        map_page(physical + (PAGE_SIZE * i), memory + (PAGE_SIZE * i),
                 PAGING_PRESENT | PAGING_USER | PAGING_WRITEABLE);
    }

    shared_memory_list[shared_memory_list_index] =
        (struct shared_memory_entry){physical, mem_len};
    shared_memory_list_index++;

    shared_memory_offset += mem_len;

    *addr = memory;

    return shared_memory_list_index - 1;
}

int shared_mem_get(int id, void **p)
{

    struct shared_memory_entry ent = shared_memory_list[id];

    void *memory = SHARED_MEMORY_START + shared_memory_offset;

    for (size_t i = 0; i < (ent.len / PAGE_SIZE) + 1; i++)
    {
        map_page(ent.mem + (PAGE_SIZE * i), memory + (PAGE_SIZE * i),
                 PAGING_PRESENT | PAGING_USER | PAGING_WRITEABLE);
    }

    *p = memory;

    return 0;
}

SYSCALL_DEFINE2(shared_mem_alloc, SYS_shared_mem_alloc)
SYSCALL_DEFINE2(shared_mem_get, SYS_shared_mem_get)
