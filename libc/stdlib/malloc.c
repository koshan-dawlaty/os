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
} Block;

void print_bin(Block **bin)
{
    Block *current = *bin;
    while (current != NULL)
    {

        printf("size: %d ", current->size);

        bin++;
        current = *bin;
    }
    printf("null\n");
}

inline void *block_to_ptr(Block *block)
{
    return block + 1;
}

inline Block *ptr_to_block(void *ptr)
{
    return ((Block *)ptr) - 1;
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

static int heap_debug = 0;

Block *get_block(size_t size)
{

#ifdef __is_libk
    // bad
    if (current_page == NULL ||
        current_page_offset + size + sizeof(Block) >= PAGE_SIZE)
    {

        current_page = get_free_pages(1);
        current_page_offset = 0;
    }

    if (heap_debug)
    {
        /* printf("page is: %p, offset is: %d, total: %p\n", current_page, */
        /*        current_page_offset, current_page + current_page_offset); */
        *(size_t *)(current_page + current_page_offset) = 'a';
        /* printf("this runs after!!\n"); */
    }

    // it is something to do with parenthesis here
    Block *block = (Block *)(current_page + current_page_offset);

    /* printf("block addr: %p\n", block); */

    block->size = size;
    block->next = NULL;

    current_page_offset += size + sizeof(Block);

    return block;

#endif
}

Block *get_block_from_bin_fast(Block **bin, int new_size)
{
    if (*bin == NULL)
    {

        Block *b = get_block(new_size);

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

            // remove next two blocks
            *prev_block = current_block->next->next;

            current_block->next->size =
                ((current_block->size + sizeof(Block)) +
                 (current_block->next->size + sizeof(Block)));

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
    int total_size = align_long2(size + sizeof(Block), ALIGN);
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

    Block *block = ptr_to_block(ptr);
    /* printf("free size is:%d\n", block->size); */
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

// TODO: VERY BAD!!!!
void *realloc(void *ptr, size_t new_size)
{

    if (ptr == NULL)
    {
        return malloc(new_size);
    }

    Block *block = ptr_to_block(ptr);
    /* printf("realloc size is:%d\n", block->size); */
    /* printf("normal %p, no %p\n", block, block - 1); */
    /* printf("realloc ptr is:%p\n", ptr); */
    void *new_ptr = malloc(new_size);
    memcpy(new_ptr, ptr, block->size);

    /* free(ptr); */

    return new_ptr;
}
