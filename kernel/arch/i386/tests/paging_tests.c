#include "interrupts/pic.h"
#include "kernel/paging.h"
#include "memory/paging_flags.h"
#include "tests/tests.h"
#include <stdint.h>
#include <stdio.h>

/*
 * returns true if test succeeded
 */
int test_map_page(uint32_t virtual, uint32_t physical)
{

    map_page((void *)physical, (void *)virtual, PAGING_WRITEABLE);

    *(char *)virtual = 'a';

    uint32_t physaddr = (uint32_t)get_physaddr((void *)virtual);

    /* printf("%p\n", get_physaddr((void *)virtual)); */
    unmap_page((void *)virtual);

    return physaddr == physical;
}

TEST(test_map_page_multiple, {
    uint32_t virtual = 0x20000000;
    uint32_t physical = 0x10000000;

    for (int i = 0; i < 10; i++)
    {
        ASSERT_TRUE(test_map_page(virtual, physical));
        virtual += PAGE_SIZE;
        physical += PAGE_SIZE;
    }
})
