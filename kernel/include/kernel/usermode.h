#ifndef USERMODE_HEADER_H
#define USERMODE_HEADER_H
#include "scheduler.h"
#include <stdint.h>

void jump_usermode();
void init_usermode();
extern void jump_usermode_asm(uint32_t addr, struct registers *regs);

#endif
