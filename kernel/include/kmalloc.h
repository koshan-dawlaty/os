#ifndef KMALLOC_H
#define KMALLOC_H

#include <malloc_internal.h>

/* this is just here to not have lsp errors when calling malloc_zone because it
 * is in a non-standard header */
static inline void *kmalloc(size_t size, const struct malloc_zone *zone)
{
    return malloc_zone(size, zone);
}

static inline void kfree(void *ptr, const struct memory_zone *zone)
{
    free_zone(ptr, zone);
}

#endif
