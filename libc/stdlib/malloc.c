#include "sys/mman.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __is_libk
#include <kernel/semaphore.h>
#include <memory/get_free_pages.h>
#include <panic.h>

#endif

// TODO: this is duplicated in kernel
long align_long2(long in, long align)
{
    return ((in / align) + 1) * align;

    if (in % align == 0)
    {
        return in;
    }

    return (in + align) - (in + 1 % align);
}

// would be 8 on 64 bit
#define ALIGN 4

typedef struct __attribute__((packed)) Block
{
    size_t size;
    struct Block *next;
} heap_block;

void print_bin(heap_block **bin)
{
    heap_block *current = *bin;
    while (current != NULL)
    {

        printf("size: %d ", current->size);

        bin++;
        current = *bin;
    }
    printf("null\n");
}

static inline void *block_to_ptr(heap_block *block)
{
    return (void *)((heap_block *)block + 1);
}

static inline heap_block *ptr_to_block(void *ptr)
{
    return ((heap_block *)ptr) - 1;
}

void *current_page = NULL;
size_t current_page_offset = 0;

#define SMALL_BIN_AMOUNT 60
heap_block *small_bins[60] = {NULL};

heap_block *unsorted_bin = NULL;

void insert_bin(heap_block **bin, heap_block *block)
{
    heap_block *old = *bin;
    *bin = block;

    (*bin)->next = old;
}

/* TODO: bad */
void *user_malloc_addr = (void *)0xA0000000;
uint32_t user_malloc_offset = 4096;

heap_block *get_block(size_t size)
{

#ifdef __is_libk
    // bad
    if (current_page == NULL ||
        current_page_offset + size + sizeof(heap_block) >= PAGE_SIZE)
    {
        current_page = get_free_pages(1);
        current_page_offset = 0;
    }

    heap_block *block = (heap_block *)(current_page + current_page_offset);

    block->size = size;
    block->next = NULL;

    current_page_offset += size + sizeof(heap_block);

    return block;

#else

    if (user_malloc_offset + size + sizeof(heap_block) >= 4096)
    {
        /* TODO: this is not the correct way to use mmap */
        user_malloc_addr += 4096;
        mmap(user_malloc_addr, 0, 0, 0, 0, 0);
        user_malloc_offset = 0;
    }

    heap_block *block = (heap_block *)(user_malloc_addr + user_malloc_offset);
    block->size = size;
    block->next = NULL;
    user_malloc_offset += size + sizeof(heap_block);
    return block;

    return (void *)0x123456;

#endif
}

heap_block *get_block_from_bin_fast(heap_block **bin, int new_size)
{
    if (*bin == NULL)
    {
        heap_block *b = get_block(new_size);

        return b;
    }

    heap_block *output = *bin;
    *bin = (*bin)->next;

    return output;
}

heap_block *get_block_from_bin(heap_block **bin, size_t new_size)
{

    if (bin == NULL)
    {
        return get_block(new_size);
    }

    heap_block *current_block = *bin;
    heap_block **prev_block = bin;

    while (current_block != NULL)
    {

        if (current_block->next != NULL &&
            (current_block->size + sizeof(heap_block)) +
                    (current_block->next->size + sizeof(heap_block)) ==
                new_size)
        {

            // remove next two blocks
            *prev_block = current_block->next->next;

            current_block->next->size =
                ((current_block->size + sizeof(heap_block)) +
                 (current_block->next->size + sizeof(heap_block)));

            return current_block->next;
        }

        // may break
        if ((current_block)->size == (size_t)align_long2(new_size, ALIGN))
        {
            (*prev_block) = current_block->next;

            return (current_block);
        }

        *prev_block = current_block;
        current_block = current_block->next;
    }

    return get_block(new_size);
}

void *malloc(size_t size)
{

    void *retval = NULL;
    int total_size = align_long2(size + sizeof(heap_block), ALIGN);

#ifdef __is_libk
    if (total_size >= PAGE_SIZE)
    {
        /* TODO: can result in some wasted memory */
        heap_block *page = get_free_pages((total_size / PAGE_SIZE) + 1);
        page->size = size;
        page->next = NULL;

        return (void *)(page) + sizeof(heap_block);
    }
#endif

    if (total_size < SMALL_BIN_AMOUNT * ALIGN)
    {

        retval = block_to_ptr(
            get_block_from_bin_fast(&(small_bins[total_size / ALIGN]), size));
    }
    else
    {
        retval = block_to_ptr(get_block_from_bin(&unsorted_bin, size));
    }
    return retval;
}

void free(void *ptr)
{

    heap_block *block = ptr_to_block(ptr);
    int total_size = align_long2(block->size + sizeof(heap_block), ALIGN);

#ifdef __is_libk
    if (total_size >= PAGE_SIZE)
    {
        free_pages(block, (block->size / PAGE_SIZE) + 1);
        return;
    }
#endif

    if (total_size < SMALL_BIN_AMOUNT * ALIGN)
    {
        insert_bin(&(small_bins[total_size / ALIGN]), block);
    }
    else
    {
        insert_bin(&unsorted_bin, block);
    }
}

// TODO: VERY BAD!!!!
void *realloc(void *ptr, size_t new_size)
{

    if (ptr == NULL)
    {
        return malloc(new_size);
    }

    heap_block *block = ptr_to_block(ptr);
    void *new_ptr = malloc(new_size);
    memcpy(new_ptr, ptr, block->size);

    if (ptr != new_ptr)
    {
        /* free(ptr); */
    }

    return new_ptr;
}
