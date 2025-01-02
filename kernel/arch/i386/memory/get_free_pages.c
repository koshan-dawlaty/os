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
// TODO: this will almost definitely conflict
// with get_free_phys_page in paging.c
#define PHYSICAL_PAGES_START 0x600000

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
            if (consecutive == 0)
            {
                consecutive_start = i;
            }

            consecutive++;
        }
        else
        {
            consecutive_start = 0;
        }

        if (consecutive >= amount)
        {
            break;
        }
    }

    void *addr = (void *)((i)*PAGE_SIZE);

    void *virtual_addr = addr + VIRTUAL_PAGES_START;
    void *physical_addr = addr + PHYSICAL_PAGES_START;

    for (size_t j = consecutive_start; j < amount + i; j++)
    {

        /* printf("virtual:%p\nphysical:%p\n", virtual_addr, physical_addr); */

        virtual_free_page_bitmap[j / 8] |= IN_USE >> (j % 8);
        map_page(physical_addr, virtual_addr, PAGING_WRITEABLE);

        physical_addr += PAGE_SIZE * j;
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
        unmap_page((void *)(addr + (i * PAGE_SIZE)));
    }
}
