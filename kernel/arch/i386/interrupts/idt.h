#ifndef IDT_H
#define IDT_H

void init_idt();

#define ISR __attribute__((interrupt, target("general-regs-only")))

#endif
