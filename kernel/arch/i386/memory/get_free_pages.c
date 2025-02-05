#include "kernel/paging.h"
#include "memory/paging_flags.h"
#include "strings.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define PAGE_SIZE 4096
// this is bad
#define NUM_PAGES 1024

#define FREE 0
#define IN_USE 1

#define VIRTUAL_PAGES_START 0xD0000000

#define free_page_bitmap_size NUM_PAGES / 8

static uint8_t virtual_free_page_bitmap[free_page_bitmap_size] = {FREE};

void *get_free_pages(size_t amount)
{
    int i;
    size_t consecutive = 0;
    size_t consecutive_start = 0;
    for (i = 0; i < NUM_PAGES; i++)
    {
        if (((virtual_free_page_bitmap[i / 8] >> (i % 8)) & 1) == FREE)
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

    void *virtual_addr = addr + VIRTUAL_PAGES_START;

    void *physaddr = get_free_phys_pages(amount);

    for (size_t j = consecutive_start; j < amount + i; j++)
    {
        /* printf("j is:%d\n", j); */

        /* printf("virtual:%p\nphysical:%p\n", virtual_addr, physical_addr); */

        virtual_free_page_bitmap[j / 8] |= (1 << (j % 8));
        map_page(physaddr + (PAGE_SIZE * j), virtual_addr, PAGING_WRITEABLE);

        virtual_addr += PAGE_SIZE * j;
    }

    return (void *)((i * PAGE_SIZE) + VIRTUAL_PAGES_START);
}

void free_pages(void *addr, size_t amount)
{

    size_t index = ((size_t)addr - VIRTUAL_PAGES_START);
    for (size_t i = 0; i < amount; i++)
    {
        virtual_free_page_bitmap[index + i / 8] |= FREE >> ((index + i) % 8);
        free_phys_pages(get_physaddr((void *)(addr + (i * PAGE_SIZE))), 1);
        unmap_page((void *)(addr + (i * PAGE_SIZE)));
    }
}

// TODO: most of this is duplicated above
#define free_page_bitmap_size NUM_PAGES / 8
static uint8_t phys_free_page_bitmap[free_page_bitmap_size] = {FREE};

#define PHYS_PAGE_START 0x1000000

void *get_free_phys_pages(size_t amount)
{
    int i;
    size_t consecutive = 0;
    size_t consecutive_start = 0;
    for (i = 0; i < NUM_PAGES; i++)
    {
        if (((phys_free_page_bitmap[i / 8] >> (i % 8)) & 1) == FREE)
        {
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

    for (size_t j = consecutive_start; j < amount + i; j++)
    {
        phys_free_page_bitmap[j / 8] |= (1 << (j % 8));
    }

    return (void *)((i * PAGE_SIZE) + PHYS_PAGE_START);
}

void free_phys_pages(void *addr, size_t amount)
{

    size_t index = ((size_t)addr - PHYS_PAGE_START);
    for (size_t i = 0; i < amount; i++)
    {
        phys_free_page_bitmap[index + i / 8] |= FREE >> ((index + i) % 8);
    }
}
