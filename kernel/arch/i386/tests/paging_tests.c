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
