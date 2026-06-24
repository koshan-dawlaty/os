#include "kernel/paging.h"
#include "memory/paging_flags.h"
#include <kernel/multiboot.h>
#include <memory/get_free_pages.h>
#include <stddef.h>
#include <string.h>
#include <types.h>

struct region_bitmap
{
    /* the virtual address of the bitmap at the start of the memory region */
    u8 *bitmap_virtual;
};

#define REGION_BITMAP_FREE 0
#define REGION_BITMAP_USED 1

static struct region_bitmap bitmaps[DEFAULT_MEMORY_REGIONS_MAX];
static struct memory_region *bitmap_regions;
static size_t num_bitmaps = 0;

void phys_pages_init(struct memory_region *regions, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        struct memory_region m = regions[i];
        /* TODO: it might be good to remove memory regions that are only a few
         * pages, although it does not seem like the bios will give any memory
         * regions like this */

        if (m.len < PAGE_SIZE * 2)
        {
            /* cant allocate from this, because the bitmap needs to be stored in
             * the first page */
            continue;
        }

        size_t bitmap_size = (m.len / PAGE_SIZE) / 8;
        size_t num_pages = (bitmap_size / PAGE_SIZE) + 1;

        void *virtual = get_free_pages_unmmaped(num_pages, &gfp_kernel_zone);
        for (size_t j = 0; j < num_pages; j++)
        {
            map_page(m.start + (j * PAGE_SIZE), virtual + (j * PAGE_SIZE),
                     PAGING_WRITEABLE | PAGING_PRESENT);
        }

        memset(virtual, REGION_BITMAP_FREE, bitmap_size);
        struct region_bitmap bitmap = {.bitmap_virtual = virtual};
        bitmaps[i] = bitmap;
    }
    num_bitmaps = len;
    bitmap_regions = regions;
}

void *get_free_phys_pages(size_t num)
{
    for (size_t j = 0; j < num_bitmaps; j++)
    {
        struct region_bitmap bitmap = bitmaps[j];
        size_t consecutive = 0;
        size_t consecutive_start = 0;
        for (size_t i = 0; i < bitmap.len * 8; i++)
        {
            if (bitmap.bitmap_virtual[i / 8] >> (i % 8)) & 1) ==
        }
    }
}
