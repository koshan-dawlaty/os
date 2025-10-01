#include "kernel/paging.h"
#include "memory/paging_flags.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define PAGE_SIZE 4096
// this is bad
#define NUM_PAGES 1024

#define FREE 0
#define IN_USE 1

#ifdef CONFIG_x86_64
#define VIRTUAL_PAGES_START 0xffffD00000000000
#else
#define VIRTUAL_PAGES_START 0xD0000000
#endif
// TODO: this will almost definitely conflict
// with get_free_phys_page in paging.c
#define PHYSICAL_PAGES_START 0x600000

#define free_page_bitmap_size NUM_PAGES / 8

static uint8_t virtual_free_page_bitmap[free_page_bitmap_size] = {FREE};

void *get_free_pages_offset(size_t amount, uint8_t *bitmap,
                            ptr base_addr_virtual, ptr base_addr_physical,
                            uint8_t flags)
{
    ptr i;
    size_t consecutive = 0;
    size_t consecutive_start = 0;
    for (i = 0; i < NUM_PAGES; i++)
    {
        if (((bitmap[i / 8] >> (i % 8)) & 1) == FREE)
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

    void *virtual_addr = addr + base_addr_virtual;
    void *physical_addr = addr + base_addr_physical;

    for (size_t j = consecutive_start; j < amount + i; j++)
    {
        /* printf("j is:%d\n", j); */

        /* printf("virtual:%p\nphysical:%p\n", virtual_addr, physical_addr); */

        bitmap[j / 8] |= (1 << (j % 8));

        map_page(physical_addr, virtual_addr, flags);

        physical_addr += PAGE_SIZE * j;
        virtual_addr += PAGE_SIZE * j;
    }

    void *address = (void *)((i * PAGE_SIZE) + base_addr_virtual);

    /* release_semaphore(get_free_pages_semaphore); */

    // asm("hlt");

    return address;
}

void free_pages_offset(void *addr, size_t amount, uint8_t *bitmap,
                       ptr base_addr)
{

    size_t index = ((size_t)addr - base_addr);
    for (size_t i = 0; i < amount; i++)
    {
        bitmap[index + i / 8] |= FREE >> ((index + i) % 8);
        unmap_page((void *)(addr + (i * PAGE_SIZE)));
    }
}

void *get_free_pages(size_t amount)
{
    return get_free_pages_offset(amount, virtual_free_page_bitmap,
                                 VIRTUAL_PAGES_START, PHYSICAL_PAGES_START,
                                 PAGING_WRITEABLE);
}

void free_pages(void *addr, size_t amount)
{
    free_pages_offset(addr, amount, virtual_free_page_bitmap,
                      VIRTUAL_PAGES_START);
}

static uint8_t user_virtual_free_page_bitmap[free_page_bitmap_size] = {FREE};
#define VIRTUAL_PAGES_USER_START 0x90000000
#define PHYSICAL_PAGES_USER_START 0xC00000

void *get_free_pages_user(size_t amount)
{

    return get_free_pages_offset(
        amount, user_virtual_free_page_bitmap, VIRTUAL_PAGES_USER_START,
        PHYSICAL_PAGES_USER_START, PAGING_WRITEABLE | PAGING_USER);
}

void free_pages_user(void *addr, size_t amount)
{
    free_pages_offset(addr, amount, user_virtual_free_page_bitmap,
                      VIRTUAL_PAGES_USER_START);
}
