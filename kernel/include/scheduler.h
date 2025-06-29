#ifndef SCHEDULER_H
#define SCHEDULER_H
#include "interrupts/idt.h"
#include "kernel/paging.h"
#include <stdint.h>

struct registers
{
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    struct interrupt_frame frame;
} __attribute__((packed));

struct task
{
    uint32_t instruction_pointer;
    /* user */
    uint32_t stack_pointer;
    struct task *next;

    struct pagedir *pgdir;
    void *stack;

    struct registers regs;
    uint32_t bp;
};

/*
 * ip is the current instruction pointer
 */
void switch_tasks(struct registers regs);
void add_task(struct task *task);

/*
 * initializes the page directory as a free page
 */
struct task *new_task(uint32_t ip, uint32_t sp);

extern void switch_tasks_asm(uint32_t ip, uint32_t kernel_sp, uint32_t user_sp,
                             uint32_t user_bp);

void enable_scheduler();
void disable_scheduler();

#endif
