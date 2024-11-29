#ifndef PAGING_H
#define PAGING_H

#include "multiboot.h"
#include <stdint.h>

void init_paging(multiboot_info_t *mbd);

void map_addr(uint32_t addr);

#endif
