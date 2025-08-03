#include "sys/mman.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __is_libk
#include <kernel/paging.h>
#endif

// TODO: this is duplicated in kernel
long align_long2(long in, long align)
{

    if (in % align == 0)
    {
        return in;
    }

    return (in + align) - (in % align);
}

// would be 8 on 64 bit
#define ALIGN 4

typedef struct Block
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

inline void *block_to_ptr(heap_block *block)
{
    return block + 1;
}

inline heap_block *ptr_to_block(void *ptr)
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

static int heap_debug = 0;

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

        if (heap_debug)
        {
            /* printf("this is running!!\n"); */

            while (1)
            {
                asm("hlt");
            }
        }

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
        /* printf("no free list\n"); */
        return get_block(new_size);
    }

    heap_block *current_block = *bin;
    heap_block **prev_block = bin;

    /* printf("size is:%d\n", new_size); */

    while (current_block != NULL)
    {

        /* printf("current: %ld\n", current_block->size + sizeof(Block)); */

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

            /* printf("matching block found\n"); */

            /* printf("current_block value: %p\n", (current_block)->val); */
            /* printf("current_block addr: %p\n", (current_block)); */
            /* printf("current_block next: %p\n", (current_block)->next); */
            /* printf("prev_block: %p\n", *prev_block); */

            (*prev_block) = current_block->next;

            /* printf("prev_block: %p\n", *prev_block); */

            return (current_block);
        }

        *prev_block = current_block;
        current_block = current_block->next;
    }

    return get_block(new_size);
}

void *malloc(size_t size)
{
    int total_size = align_long2(size + sizeof(heap_block), ALIGN);
    /* printf("total size:%d\n", total_size / ALIGN); */

    if (total_size < SMALL_BIN_AMOUNT * ALIGN)
    {

        return block_to_ptr(
            get_block_from_bin_fast(&(small_bins[total_size / ALIGN]), size));
    }
    else
    {
        return block_to_ptr(get_block_from_bin(&unsorted_bin, size));
    }
}

void free(void *ptr)
{

    heap_block *block = ptr_to_block(ptr);
    /* printf("free size is:%d\n", block->size); */
    int total_size = block->size + sizeof(heap_block);

    /* printf("block size:%d\n", block->size); */
    /* printf("total size:%d\n", total_size / ALIGN); */

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
    /* printf("realloc size is:%d\n", block->size); */
    /* printf("normal %p, no %p\n", block, block - 1); */
    /* printf("realloc ptr is:%p\n", ptr); */
    void *new_ptr = malloc(new_size);
    memcpy(new_ptr, ptr, block->size);

    /* free(ptr); */

    return new_ptr;
}
