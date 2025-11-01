#ifndef GDT_H
#define GDT_H
#include "kernel/types.h"
#include <stdint.h>

void init_gdt();
void set_kernel_stack(ptr stack);

#endif
