#ifndef GDT_H
#define GDT_H
#include <stdint.h>
#include <types.h>

void init_gdt();
void set_kernel_stack(ptr stack);

#endif
