#include <kernel/paging.h>
#include <kernel/types.h>
#include <stddef.h>

struct memory_zone
{
    ptr virtual;
    ptr physical;

    ptr size; /* the size of this zone in pages */

    u8 *bitmap;
    u8 pg_flags; /* paging flags to be used with this zone */

    /* TODO: add a lock here */
};

/* TODO: define this somewhere in architecture */
#define PAGE_SIZE 4096

#define GFP_FREE 0
#define GFP_IN_USE 1

const struct memory_zone gpf_kernel_zone = {
    .virtual = 0xffffD00000000000,
    .physical = 0x6000000,

};

void *get_free_pages_zone(size_t amount, struct memory_zone *zone)
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
