#include "kernel/paging.h"
#include "kernel/utils.h"
#include <stdio.h>

// would be 8 on 64 bit
#define ALIGN 4

typedef struct Block
{
    size_t size;
    struct Block *next;
} Block;

inline void *block_to_ptr(Block *block)
{
    return block + sizeof(Block);
}

inline Block *ptr_to_block(void *ptr)
{
    return (Block *)ptr - sizeof(Block);
}

void *current_page = NULL;
size_t current_page_offset = 0;

#define SMALL_BIN_AMOUNT 60
Block *small_bins[60] = {NULL};

Block *unsorted_bin = NULL;

void insert_bin(Block **bin, Block *block)
{

    Block *old = *bin;
    *bin = block;

    (*bin)->next = old;
}

Block *get_block(size_t size)
{
    // bad
    if (current_page == NULL ||
        current_page_offset + size + sizeof(Block) >= PAGE_SIZE)
    {
        current_page = get_free_pages(1);
    }

    Block *block = (Block *)current_page + current_page_offset;
    block->size = size;
    block->next = NULL;

    current_page_offset += size + sizeof(Block);

    return block;
}

Block *get_block_from_bin_fast(Block **bin, int new_size)
{
    if (*bin == NULL)
    {

        return get_block(new_size);
    }

    Block *output = *bin;
    *bin = (*bin)->next;

    return output;
}

Block *get_block_from_bin(Block **bin, size_t new_size)
{

    if (bin == NULL)
    {
        /* printf("no free list\n"); */
        return get_block(new_size);
    }

    Block *current_block = *bin;
    Block **prev_block = bin;

    /* printf("size is:%d\n", new_size); */

    while (current_block != NULL)
    {

        /* printf("current: %ld\n", current_block->size + sizeof(Block)); */

        if (current_block->next != NULL &&
            (current_block->size + sizeof(Block)) +
                    (current_block->next->size + sizeof(Block)) ==
                new_size)
        {

            /* printf("condition true\n"); */

            // remove next two blocks
            *prev_block = current_block->next->next;

            current_block->next->size =
                ((current_block->size + sizeof(Block)) +
                 (current_block->next->size + sizeof(Block)));

            return current_block->next;
        }

        // may break
        if ((current_block)->size == (size_t)align_long(new_size, ALIGN))
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
    /* printf("no matching block\n"); */

    return get_block(new_size);
}

void *kmalloc(size_t size)
{

    int total_size = align_long(size + sizeof(Block), ALIGN);
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

void kfree(void *ptr)
{

    Block *block = ptr_to_block(ptr);
    int total_size = block->size + sizeof(Block);

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
