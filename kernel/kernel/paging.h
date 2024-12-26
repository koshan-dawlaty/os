#ifndef PAGING_H
#define PAGING_H

#include "multiboot.h"
#include <stdint.h>

void init_paging();

void *get_physaddr(void *virtualaddr);
void map_addr(uint32_t addr);

#endif
