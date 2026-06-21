#ifndef GET_FREE_PAGES_H
#define GET_FREE_PAGES_H

#include "lib/binary_tree.h"
#include "spinlock.h"
#include <lib/list.h>
#include <memory/paging_flags.h>
#include <stddef.h>
#include <types.h>

/* TODO: define this somewhere in an arch file */
#define PAGE_SIZE 4096

#define GFP_FREE 0
#define GFP_IN_USE 1

struct memory_zone
{

    void *(*get_pages)(size_t, struct memory_zone *);
    void (*free_pages)(void *, size_t, struct memory_zone *);

    void *zone_private;

    struct spinlock lock;
};

#define MEMORY_ZONE_INIT(get, free, private)                                   \
    (struct memory_zone)                                                       \
    {                                                                          \
        .get_pages = get, .free_pages = free, .zone_private = private,         \
        .lock = SPINLOCK_INIT()                                                \
    }

struct bitmap_memory_zone
{
    ptr_t virtual;
    ptr_t physical;

    ptr_t size; /* the size of this zone in pages */

    u8 *bitmap;
    u8 pg_flags; /* paging flags to be used with this zone */
};

extern struct memory_zone gfp_kernel_zone;
extern struct memory_zone gfp_kernel_tree_zone;
extern struct memory_zone gfp_user_zone;

extern bool gfp_zones_initialized;

/*
 * @amount: the number of pages to allocate
 * @zone: the zone to allocated the pages from
 *
 * returns address of allocated memory
 */
static inline void *get_free_pages_zone(size_t amount, struct memory_zone *zone)
{

    return zone->get_pages(amount, zone);
}

/*
 * @addr the address of the pages to free (returned by get_free_pages())
 * @amount the number of pages to free (passed to get_free_pages())
 * @zone the zone that the pages came from
 */
static inline void free_pages_zone(void *addr, size_t amount,
                                   struct memory_zone *zone)
{
    zone->free_pages(addr, amount, zone);
}

void *get_free_pages_bitmap(size_t amount, struct memory_zone *zone);
void free_pages_bitmap(void *addr, size_t amount, struct memory_zone *zone);

#define BITMAP_TO_GFP_ZONE(zone)                                               \
    MEMORY_ZONE_INIT(get_free_pages_bitmap, free_pages_bitmap, zone)

inline struct memory_zone bitmap_to_gfp_zone(struct bitmap_memory_zone *zone)
{
    return (struct memory_zone){.zone_private = (void *)zone,
                                .get_pages = get_free_pages_bitmap,
                                .free_pages = free_pages_bitmap};
}

struct tree_memory_zone
{
    struct bintree_head head;
    void *base;
    size_t offset; /* offset, in pages from base */
};
void *get_free_pages_tree(size_t amount, struct memory_zone *z);
void free_pages_tree(void *addr, size_t amount, struct memory_zone *z);

#define TREE_ZONE_TO_GFP_ZONE(zone)                                            \
    (struct memory_zone){.zone_private = (void *)zone,                         \
                         .get_pages = get_free_pages_tree,                     \
                         .free_pages = free_pages_tree};

/*
 * initalizes all of the zones that dont use a bitmap in the kernel binary
 */
void init_gfp_zones();

/*
 * gets free pages from the bitmap in @zone, but does not map them
 */
void *get_free_pages_unmmaped(size_t amount, struct memory_zone *zone);

static inline void *get_free_pages(size_t amount)
{
    return get_free_pages_zone(amount, &gfp_kernel_zone);
}

static inline void free_pages(void *addr, size_t amount)
{
    free_pages_zone(addr, amount, &gfp_kernel_zone);
}

static inline void *get_free_pages_user(size_t amount)
{
    return get_free_pages_zone(amount, &gfp_user_zone);
}

static inline void free_pages_user(void *addr, size_t amount)
{
    free_pages_zone(addr, amount, &gfp_user_zone);
}

#endif
