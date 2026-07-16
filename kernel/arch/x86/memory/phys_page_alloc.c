#include "kernel/paging.h"
#include "lib/assert.h"
#include "lib/log.h"
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
static struct memory_region bitmap_regions[DEFAULT_MEMORY_REGIONS_MAX];
#define REGION_BITMAP_SIZE(region) (((region).len / PAGE_SIZE) / 8)
#define REGION_BITMAP_PAGES(region) (((region).len / PAGE_SIZE) + 1)
#define BITMAP_BASE(region)                                                    \
    (void *)((((REGION_BITMAP_SIZE(region) + (ptr_t)region.start) /            \
               PAGE_SIZE) +                                                    \
              1) *                                                             \
             PAGE_SIZE)

static size_t num_bitmaps = 0;

void phys_pages_init(struct memory_region *regions, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        struct memory_region m = regions[i];
        /* FIXME: hardcoded, should be past the end of low memory, this is just
         * the first valid bitmap we normally get */
        if (m.start < (void *)100000)
        {
            continue;
        }

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
        bitmaps[num_bitmaps] = bitmap;
        bitmap_regions[num_bitmaps] = m;
        num_bitmaps++;
    }
}

void *get_free_phys_pages2(size_t num)
{
    struct region_bitmap bitmap;
    struct memory_region region;
    size_t consecutive_start;
    size_t i;
    for (size_t j = 0; j < num_bitmaps; j++)
    {
        bitmap = bitmaps[j];
        region = bitmap_regions[j];
        size_t consecutive = 0;
        consecutive_start = 0;
        size_t num_pages = REGION_BITMAP_PAGES(region);
        for (i = 0; i < num_pages; i++)
        {
            if (((bitmap.bitmap_virtual[i / 8] >> (i % 8)) & 1) ==
                REGION_BITMAP_FREE)
            {
                logd("found free at:%d", i);
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
            if (consecutive >= num)
            {
                goto have_page;
            }
        }
    }
    PANIC("get_free_phys_pages() ran out of memory in %d bitmap(s)",
          num_bitmaps);

    return NULL;

have_page:;
    logd("allocating at:%d", consecutive_start);
    for (size_t j = consecutive_start; j < num + i; j++)
    {
        bitmap.bitmap_virtual[j / 8] |= (1 << (j % 8));
    }

    void *bitmap_base = BITMAP_BASE(region);

    void *addr = (void *)((consecutive_start * PAGE_SIZE) + bitmap_base);
    logd("ret:%p, %p i:%d", addr, bitmap_base, i);
    return addr;
}

void free_phys_pages2(void *addr, size_t num)
{
    for (size_t j = 0; j < num_bitmaps; j++)
    {
        struct memory_region region = bitmap_regions[j];
        struct region_bitmap bitmap = bitmaps[j];
        logd("addr:%p, %p", addr, region.start);
        /* find what region @addr is in, they can't overlap so we dont care
         * about @num */
        if (addr >= region.start && addr < region.start + region.len)
        {
            size_t index = (addr - BITMAP_BASE(region)) / PAGE_SIZE;
            logd("freeing at:%p", index);

            for (size_t i = 0; i < num; i++)
            {
                bitmap.bitmap_virtual[(index + i) / 8] &=
                    ~(1 << (index + i) % 8);
            }
        }
    }
}
