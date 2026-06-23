#include "kernel/multiboot.h"
#include <kernel/paging.h>
#include <kernel/syscall.h>
#include <kernel/utils.h>
#include <lib/log.h>
#include <memory/get_free_pages.h>
#include <memory/paging_flags.h>
#include <spinlock.h>
#include <stdio.h>
#include <syscall.h>

struct shared_memory_entry
{
    void *mem;
    /* the virtal address of the entry in the caller's address space */
    void *creator_addr;
    size_t len;
};

/* TODO: make this dynamic */
#define SHARED_MEMORY_MAX 100
struct shared_memory_entry shared_memory_list[SHARED_MEMORY_MAX];
struct spinlock shared_mem_lock = SPINLOCK_INIT();
size_t shared_memory_list_index = 0;

#define SHARED_MEMORY_START (void *)0x30000000
size_t shared_memory_offset = 0;

int shared_mem_alloc(void **addr, size_t len)
{
    spinlock_lock(&shared_mem_lock);

    void *memory = SHARED_MEMORY_START + shared_memory_offset;
    size_t mem_len = ((len / PAGE_SIZE) + 1) * PAGE_SIZE;
    size_t num_pages = (mem_len / PAGE_SIZE) + 2;
    void *physical = get_free_phys_pages(get_current_pagedir(), num_pages);

    /* printf("physaddr:%p-%p, %d\n", physical, physical + len, num_pages); */
    /* printf("virtaddr:%p-%p, %d\n", memory, memory + num_pages * PAGE_SIZE, */
    /* num_pages); */
    for (size_t i = 0; i < num_pages; i++)
    {
        map_page(physical + (PAGE_SIZE * i), memory + (PAGE_SIZE * i),
                 PAGING_PRESENT | PAGING_USER | PAGING_WRITEABLE);
    }

    shared_memory_list[shared_memory_list_index] =
        (struct shared_memory_entry){physical, memory, mem_len};
    shared_memory_list_index++;
    shared_memory_offset += mem_len;

    *addr = memory;

    spinlock_unlock(&shared_mem_lock);
    return shared_memory_list_index - 1;
}

int shared_mem_get(int id, void **p)
{

    spinlock_lock(&shared_mem_lock);
    struct shared_memory_entry ent = shared_memory_list[id];

    void *memory = SHARED_MEMORY_START + shared_memory_offset;
    shared_memory_offset += ((ent.len / PAGE_SIZE) + 3) * PAGE_SIZE;
    /* printf("get:%d\n", ent.len); */
    size_t num_pages = (ent.len / PAGE_SIZE) + 3;

    /* printf("get:%d\n", num_pages); */
    for (size_t i = 0; i < num_pages; i++)
    {
        map_page(ent.mem + (PAGE_SIZE * i), memory + (PAGE_SIZE * i),
                 PAGING_PRESENT | PAGING_USER | PAGING_WRITEABLE);
    }

    *p = memory;
    logd("got address:%p %d", memory, ent.len / PAGE_SIZE);
    spinlock_unlock(&shared_mem_lock);

    return 0;
}

int shared_mem_resize(int id, size_t new_size, void **new_addr)
{
    spinlock_lock(&shared_mem_lock);

    struct shared_memory_entry *ent = &shared_memory_list[id];

    size_t old_size = ent->len;

    logd("new:%p old:%p", new_size, old_size);
    if (new_size > old_size)
    {
        void *memory = SHARED_MEMORY_START + shared_memory_offset;
        size_t num_pages = (new_size / PAGE_SIZE) + 3;
        logd("num_pages:%d, old:%d", num_pages, (old_size / PAGE_SIZE) + 3);
        shared_memory_offset += (num_pages + PAGE_SIZE) * PAGE_SIZE;
        void *physical = get_free_phys_pages(get_current_pagedir(), num_pages);

        for (size_t i = 0; i < num_pages; i++)
        {
            map_page(physical + (PAGE_SIZE * i), memory + (PAGE_SIZE * i),
                     PAGING_PRESENT | PAGING_USER | PAGING_WRITEABLE);
        }

        size_t num_old_pages = (old_size / PAGE_SIZE) + 3;

        free_phys_pages(ent->mem, num_old_pages);
        memcpy(memory, ent->creator_addr, old_size);
        ent->creator_addr = memory;

        ent->mem = physical;

        *new_addr = memory;
    }
    else
    {
        *new_addr = NULL;
    }

    ent->len = new_size;

    logd("done resizing memory, addr is:%p %d", *new_addr,
         ent->len / PAGE_SIZE);
    spinlock_unlock(&shared_mem_lock);
    return 0;
}

SYSCALL_DEFINE3(SYS_shared_mem_resize, int, int, id, size_t, new_size, void **,
                new_addr)
{
    return shared_mem_resize(id, new_size, new_addr);
}

/* TODO: */
SYSCALL_DEFINE2(SYS_shared_mem_alloc, int, void **, addr, size_t, len)
{
    return shared_mem_alloc(addr, len);
}

SYSCALL_DEFINE2(SYS_shared_mem_get, int, int, id, void **, p)
{
    return shared_mem_get(id, p);
}
