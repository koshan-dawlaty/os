#ifndef MALLOC_INTERNAL_H
#define MALLOC_INTERNAL_H

#include <stddef.h>

typedef struct __attribute__((packed)) heap_block
{
    size_t size;
    struct heap_block *next;
} heap_block;

#define SMALL_BIN_AMOUNT 60
struct malloc_zone
{
    struct memory_zone *zone;
    void *current_page;
    void *initial_page;
    size_t page_offset;

    heap_block *small_bins[SMALL_BIN_AMOUNT];

    heap_block *unsorted_bin;
};

#define INIT_MALLOC_ZONE(_zone)                                                \
    (struct malloc_zone){                                                      \
        .zone = _zone,                                                         \
        .current_page = NULL,                                                  \
        .initial_page = NULL,                                                  \
        .page_offset = 0,                                                      \
        .small_bins = {NULL},                                                  \
        .unsorted_bin = NULL,                                                  \
    };

void *malloc_zone(size_t size, struct malloc_zone *zone);
void free_zone(void *ptr, struct malloc_zone *zone);

#ifdef __is_libk
extern struct malloc_zone kernel_malloc_zone;
extern struct malloc_zone kernel_tree_malloc_zone;
extern struct malloc_zone user_malloc_zone;
#else
extern struct malloc_zone libc_malloc_zone;
#endif

#endif
