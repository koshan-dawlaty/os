#ifndef USERMODE_HEADER_H
#define USERMODE_HEADER_H
#include <stdint.h>

void jump_usermode();
void init_usermode();
extern void jump_usermode_asm(uint32_t addr);

#endif
