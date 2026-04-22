#include <extension.h>
#include <malloc_internal.h>
#include <sched.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#ifdef __is_libk
#include <memory/get_free_pages.h>
#include <panic.h>
#include <spinlock.h>
#endif

/* used for heap corruptions */
void malloc_error(const char *message)
{
#ifdef __is_libk
    panic_no_malloc(message);
#else
    printf("heap error\n");
    printf(message);
    exit(1);
#endif
}

// TODO: this is duplicated in kernel
long align_long2(long in, long align)
{
    /* return ((in / align) + 1) * align; */

    /* if (in % align == 0) */
    /* { */
    /*     return in; */
    /* } */

    return (in + align - 1) & ~(align - 1);
}

// would be 8 on 64 bit
#define ALIGN 4

#ifdef __is_libk
struct malloc_zone kernel_malloc_zone = INIT_MALLOC_ZONE(&gfp_kernel_zone);
struct malloc_zone kernel_tree_malloc_zone =
    INIT_MALLOC_ZONE(&gfp_kernel_tree_zone);

/* struct malloc_zone kernel_tree_malloc_zone = */
/*     INIT_MALLOC_ZONE(&gfp_kernel_tree_zone); */
struct malloc_zone user_malloc_zone = INIT_MALLOC_ZONE(&gfp_user_zone);
#else
struct malloc_zone libc_malloc_zone = INIT_MALLOC_ZONE(NULL)
#endif

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

void insert_bin(heap_block **bin, heap_block *block)
{
    heap_block *old = *bin;
    *bin = block;
    (*bin)->next = old;
}

/* TODO: bad */
void *user_malloc_addr = (void *)0xA0000000;
uint64_t user_malloc_offset = 4096;

void *malloc_get_pages(size_t num, struct malloc_zone *zone)
{
#ifdef __is_libk
    return get_free_pages_zone(num, zone->zone);
#else
    user_malloc_offset += 4096;
    void *out =
        mmap(user_malloc_addr + user_malloc_offset, num * 4096, 0, 0, 0);
    user_malloc_offset += num * 4096;
    return out;
#endif
}

void malloc_free_pages(void *addr, size_t num, struct malloc_zone *zone)
{
#ifdef __is_libk
    free_pages_zone(addr, num, zone->zone);
#else
    /* TODO: libc should probably be able to free pages */

#endif
}

/* ensures that a block is correct and the heap has not been corrupted */
void check_block(heap_block *block)
{
    /* if (((void *)block->next < initial_page || */
    /*      (void *)block->next > current_page + 4096) && */
    /*     block->next != NULL) */
    /* { */
    /*     malloc_error("invalid addr"); */
    /* } */
}

heap_block *get_block(size_t size, struct malloc_zone *zone)
{

    if (zone->current_page == NULL ||
        zone->page_offset + size + sizeof(heap_block) >= 4096)
    {

        if (zone->current_page == NULL)
        {
            zone->initial_page = malloc_get_pages(1, zone);
            zone->current_page = zone->initial_page;
        }
        else
        {
            zone->current_page = malloc_get_pages(1, zone);
            zone->page_offset = 0;
        }
    }

    heap_block *block =
        (heap_block *)((uintptr_t)zone->current_page + zone->page_offset);

    size_t total_size = align_long2(size + sizeof(heap_block), ALIGN);
    block->size = total_size;
    block->next = NULL;

    /* zone->page_offset += size + sizeof(heap_block); */
    zone->page_offset += total_size;

    return block;
}

heap_block *get_block_from_bin_fast(heap_block **bin, int new_size,
                                    struct malloc_zone *zone)
{
    if (*bin == NULL)
    {
        heap_block *b = get_block(new_size, zone);

        return b;
    }

    heap_block *output = *bin;

    *bin = output->next;
    output->next = NULL;

    return output;
}

heap_block *get_block_from_bin(heap_block **bin, size_t new_size,
                               struct malloc_zone *zone)
{

    if (bin == NULL)
    {
        return get_block(new_size, zone);
    }

    heap_block *current_block = *bin;
    heap_block **prev_block = bin;

    while (current_block != NULL)
    {

        /* if (current_block->next != NULL && */
        /*     (current_block->size + sizeof(heap_block)) + */
        /*             (current_block->next->size + sizeof(heap_block)) == */
        /*         new_size) */
        /* { */

        /*     // remove next two blocks */
        /*     *prev_block = current_block->next->next; */

        /*     current_block->next->size = */
        /*         ((current_block->size + sizeof(heap_block)) + */
        /*          (current_block->next->size + sizeof(heap_block))); */

        /*     return current_block->next; */
        /* } */

        if ((current_block)->size ==
            (size_t)align_long2(new_size + sizeof(struct heap_block), ALIGN))
        {
            (*prev_block) = current_block->next;

            return (current_block);
        }

        *prev_block = current_block;
        current_block = current_block->next;
    }

    return get_block(new_size, zone);
}

void free_zone(void *ptr, struct malloc_zone *zone)
{
    if (ptr == NULL)
    {
        return;
    }

    heap_block *block = ptr_to_block(ptr);

    if (block->next != NULL)
    {
        malloc_error("double free\n");
    }

    check_block(block);

    /* this seemed to be causing problems where some allocations (maybe above
     * 4096 or SMALL_BIN_AMOUNT * ALIGN?) were getting overwritten */
    /* int total_size = align_long2(block->size + sizeof(heap_block), ALIGN); */
    int total_size = block->size;

    if (total_size >= 4096)
    {
        malloc_free_pages(block, (block->size / 4096) + 1, zone);
        return;
    }

    if (total_size < SMALL_BIN_AMOUNT * ALIGN)
    {
        insert_bin(&(zone->small_bins[total_size / ALIGN]), block);
    }
    else
    {
        insert_bin(&zone->unsorted_bin, block);
    }

    if (block->size > sizeof(struct heap_block))
    {
        memset(ptr, 'a', block->size - sizeof(struct heap_block));
    }
}

void *malloc_zone(size_t size, struct malloc_zone *zone)
{

    void *retval = NULL;
    int total_size = align_long2(size + sizeof(heap_block), ALIGN);

    if (total_size >= 4096)
    {
        /* TODO: can result in some wasted memory */
        heap_block *page = malloc_get_pages((total_size / 4096) + 1, zone);
        page->size = total_size;
        page->next = NULL;

        retval = (void *)(page) + sizeof(heap_block);
        goto end;
    }

    heap_block *block = NULL;

    if (total_size < SMALL_BIN_AMOUNT * ALIGN)
    {
        block = get_block_from_bin_fast(&(zone->small_bins[total_size / ALIGN]),
                                        size, zone);
    }
    else
    {
        block = get_block_from_bin(&zone->unsorted_bin, size, zone);
    }

    block->next = NULL;
    retval = block_to_ptr(block);

end:
    return retval;
}

#ifdef __is_libk
struct spinlock heap_lock = SPINLOCK_INIT();
#else
int heap_mutex = -1;
#endif

static inline void lock_heap()
{
#ifdef __is_libk
    spinlock_lock(&heap_lock);
#else
    if (heap_mutex != -1)
    {
        user_mutex_lock(heap_mutex);
    }
#endif
}
static inline void unlock_heap()
{
#ifdef __is_libk
    spinlock_unlock(&heap_lock);
#else
    if (heap_mutex != -1)
    {
        user_mutex_unlock(heap_mutex);
    }
#endif
}

void *malloc(size_t size)
{

    lock_heap();
    /* printf("lock done\n"); */

#ifdef __is_libk
    void *retval;
    if (gfp_zones_initialized)
    {
        retval = malloc_zone(size, &kernel_tree_malloc_zone);
    }
    else
    {
        retval = malloc_zone(size, &kernel_malloc_zone);
    }
#else
    void *retval = malloc_zone(size, &libc_malloc_zone);
#endif

    unlock_heap();

    return retval;
}

void free(void *ptr)
{

    lock_heap();

#ifdef __is_libk

    if (gfp_zones_initialized)
    {
        free_zone(ptr, &kernel_tree_malloc_zone);
    }
    else
    {
        free_zone(ptr, &kernel_malloc_zone);
    }
#else

    free_zone(ptr, &libc_malloc_zone);
#endif
    unlock_heap();
}

void *realloc_zone(void *ptr, size_t new_size, struct malloc_zone *zone)
{

    if (ptr == NULL)
    {
        return malloc(new_size);
    }

    lock_heap();

    void *new_ptr = malloc_zone(new_size, zone);

    heap_block *block = ptr_to_block(ptr);

    size_t copy_size = (block->size - sizeof(heap_block)) < new_size
                           ? (block->size - sizeof(heap_block))
                           : new_size;
    memcpy(new_ptr, ptr, copy_size);

    void *retval = new_ptr;

    free_zone(ptr, zone);

    unlock_heap();

    return retval;
}

void *realloc(void *ptr, size_t new_size)
{
#ifdef __is_libk
    if (gfp_zones_initialized)
    {
        return realloc_zone(ptr, new_size, &kernel_tree_malloc_zone);
    }
    else
    {
        return realloc_zone(ptr, new_size, &kernel_malloc_zone);
    }
#else
    return realloc_zone(ptr, new_size, &libc_malloc_zone);
#endif
}

void init_heap()
{
#ifdef __is_libk
    /* already initalized, maybe remove this call */
#else
    heap_mutex = user_mutex_create();
#endif
}

void *calloc(size_t n, size_t size)
{
    void *out = malloc(n * size);
    bzero(out, n * size);

    return out;
}
