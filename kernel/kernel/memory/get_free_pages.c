#include "memory/paging_flags.h"
#include <kernel/paging.h>
#include <kernel/types.h>
#include <lib/list.h>
#include <memory/get_free_pages.h>
#include <stddef.h>
#include <stdlib.h>

/*
 * @amount: the number of pages to allocate
 * @zone: the zone to allocated the pages from
 *
 * returns address of allocated memory
 */
void *get_free_pages_zone(size_t amount, const struct memory_zone *zone)
{

    ptr i;
    size_t consecutive = 0;
    size_t consecutive_start = 0;
    for (i = 0; i < zone->size; i++)
    {
        if (((zone->bitmap[i / 8] >> (i % 8)) & 1) == GFP_FREE)
        {
            /* printf("i is:%d\n", i); */
            if (consecutive == 0)
            {
                consecutive_start = i;
            }

            consecutive++;
        }
        else
        {
            consecutive_start = 0;
            consecutive = 0;
        }

        if (consecutive >= amount)
        {
            break;
        }
    }

    /* printf("start:%d\n", consecutive_start); */

    void *addr = (void *)((i)*PAGE_SIZE);

    void *virtual_addr = addr + zone->virtual;
    void *physical_addr = addr + zone->physical;

    for (size_t j = consecutive_start; j < amount + i; j++)
    {
        /* printf("j is:%d\n", j); */

        /* printf("virtual:%p\nphysical:%p\n", virtual_addr, physical_addr); */

        zone->bitmap[j / 8] |= (1 << (j % 8));

        map_page(physical_addr, virtual_addr, zone->pg_flags);

        physical_addr += PAGE_SIZE * j;
        virtual_addr += PAGE_SIZE * j;
    }

    void *address = (void *)((i * PAGE_SIZE) + zone->virtual);

    /* release_semaphore(get_free_pages_semaphore); */

    return address;
}

void free_pages_zone(void *addr, size_t amount, const struct memory_zone *zone)
{

    size_t index = ((size_t)addr - zone->virtual);
    for (size_t i = 0; i < amount; i++)
    {
        zone->bitmap[(index + i) / 8] |= GFP_FREE >> ((index + i) % 8);
        unmap_page((void *)(addr + (i * PAGE_SIZE)));
    }
}

/*
 * initalizes all of the zones that dont use a bitmap in the kernel binary
 */
void init_gfp_zones()
{
    gfp_user_zone.bitmap = malloc(gfp_user_zone.size / 8);
}
