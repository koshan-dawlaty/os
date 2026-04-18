#ifndef KMALLOC_H
#define KMALLOC_H

#include <malloc_internal.h>

#ifdef __is_libk
#define KMALLOC_BASIC &kernel_malloc_zone
#define KMALLOC_KERNEL &kernel_tree_malloc_zone
#define KMALLOC_USER &user_malloc_zone
#else
#define KMALLOC_KERNEL NULL
#define KMALLOC_USER NULL
#define KMALLOC_BASIC NULL
#endif

/* this is just here to not have lsp errors when calling malloc_zone because it
 * is in a non-standard header */
static inline void *kmalloc(size_t size, struct malloc_zone *zone)
{
    return malloc_zone(size, zone); /* might not lock the heap */
}

static inline void kfree(void *ptr, struct malloc_zone *zone)
{
    free_zone(ptr, zone);
}

#endif
