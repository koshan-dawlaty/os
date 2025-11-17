#ifndef GET_FREE_PAGES_H
#define GET_FREE_PAGES_H

#include <kernel/types.h>
#include <lib/list.h>
#include <memory/paging_flags.h>
#include <stddef.h>

/* TODO: define this somewhere in an arch file */
#define PAGE_SIZE 4096

#define GFP_FREE 0
#define GFP_IN_USE 1

struct memory_zone
{
    ptr virtual;
    ptr physical;

    ptr size; /* the size of this zone in pages */

    u8 *bitmap;
    u8 pg_flags; /* paging flags to be used with this zone */

    list2 l;

    /* TODO: add a lock here */
};

extern const struct memory_zone gfp_kernel_zone;
extern struct memory_zone gfp_user_zone;

/*
 * @amount: the number of pages to allocate
 * @zone: the zone to allocated the pages from
 *
 * returns address of allocated memory
 */
void *get_free_pages_zone(size_t amount, const struct memory_zone *zone);

/*
 * @addr the address of the pages to free (returned by get_free_pages())
 * @amount the number of pages to free (passed to get_free_pages())
 * @zone the zone that the pages came from
 */
void free_pages_zone(void *addr, size_t amount, const struct memory_zone *zone);

/*
 * initalizes all of the zones that dont use a bitmap in the kernel binary
 */
void init_gfp_zones();

/*
 * gets free pages from the bitmap in @zone, but does not map them
 */
void *get_free_pages_unmmaped(size_t amount, const struct memory_zone *zone);

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
