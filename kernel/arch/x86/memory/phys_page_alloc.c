#include "compiler.h"
#include "kernel/paging.h"
#include "kernel/utils.h"
#include "lib/assert.h"
#include "lib/log.h"
#include "memory/paging_flags.h"
#include <kernel/multiboot.h>
#include <memory/get_free_pages.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <types.h>

struct region_bitmap
{
    /* the virtual address of the bitmap at the start of the memory region */
    u8 *bitmap_virtual;
};

/* x86-64 only, address that maps all physical addresses  */
#define DIRECT_MAP_x64 0xffffc00000000000

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

bool free_pages_initalized = false;
void *phys_page_inital_addr = NULL;
void phys_pages_init(struct memory_region *regions, size_t len)
{

    /* this has to be set up because map_page() will call get_free_phys_page()
     */
    struct memory_region max = regions[0];
    for (size_t i = 0; i < len; i++)
    {
        struct memory_region r = regions[i];

        if (r.len > max.len)
        {
            max = r;
        }
    }
    /* TODO: this could theoretically overflow (maybe) */
    phys_page_inital_addr =
        (void *)((((ptr_t)(max.start) / PAGE_SIZE) + 1) * PAGE_SIZE);

    for (size_t i = 0; i < len; i++)
    {

        if (regions[i].start == (void *)0x100000)
        {
            if (regions[i].start == max.start)
            {
                /* FIXME this shouldn't be hardcoded */
                regions[i].start += 0x10000;
            }
            regions[i].start += KERNEL_SIZE;
            regions[i].start += 0x600000;
            phys_page_inital_addr =
                (void *)((((ptr_t)(regions[i].start - 0x10000) / PAGE_SIZE) +
                          1) *
                         PAGE_SIZE);
            regions[i].start =
                (void *)align_long((ptr_t)regions[i].start, PAGE_SIZE);
        }
        struct memory_region m = regions[i];

        /* FIXME: hardcoded, should be past the end of low memory, this is just
         * the first valid bitmap we normally get */
        if (m.start < (void *)0x100000)
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
        logd("m.start:%p", m.start);
        logd("m.len:%p", m.len);

        void *virtual = get_free_pages_unmmaped(num_pages, &gfp_kernel_zone);
        logd("virutal:%p", virtual);
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
    free_pages_initalized = true;
}

void *get_free_phys_pages(size_t num)
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
                /* logd("found free at:%d", i); */
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
    /* logd("allocating at:%d", consecutive_start); */
    for (size_t j = consecutive_start; j < num + i; j++)
    {
        bitmap.bitmap_virtual[j / 8] |= (1 << (j % 8));
    }

    void *bitmap_base = BITMAP_BASE(region);

    void *addr = (void *)((consecutive_start * PAGE_SIZE) + bitmap_base);

#if defined(CONFIG_x86_64) && defined(CONFIG_CLEAR_PAGES)
    memset(addr + DIRECT_MAP_x64, 'b', PAGE_SIZE * num);
#endif
    return addr;
}

void free_phys_pages(void *addr, size_t num)
{
    for (size_t j = 0; j < num_bitmaps; j++)
    {
        struct memory_region region = bitmap_regions[j];
        struct region_bitmap bitmap = bitmaps[j];

        /* find what region @addr is in, they can't overlap so we dont care
         * about @num */
        if (addr >= region.start && addr < region.start + region.len)
        {
            size_t index = (addr - BITMAP_BASE(region)) / PAGE_SIZE;

            for (size_t i = 0; i < num; i++)
            {
                bitmap.bitmap_virtual[(index + i) / 8] &=
                    ~(1 << (index + i) % 8);
            }
        }
    }
}

void *get_free_phys_page()
{

    /* this is called by map_page, which is called during initalization of this,
     * so this has to be done */
    if (unlikely(!free_pages_initalized))
    {
        void *out = phys_page_inital_addr;
        phys_page_inital_addr += PAGE_SIZE;
        return out;
    }
    else
    {
        return get_free_phys_pages(1);
    }
}
