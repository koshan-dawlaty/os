#ifndef SCHEDULER_H
#define SCHEDULER_H
#include "kernel/paging.h"
#include <stdbool.h>
#include <stdint.h>

struct registers
{
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t eflags;

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

void switch_tasks();
void add_task(struct task *task);

/*
 * initializes the page directory as a free page
 */
struct task *new_task(uint32_t ip, uint32_t sp);

/*
 * creates a task for the currently running kernel code
 * intended to be used to create a task for the code running initially
 */
struct task *create_kernel_task();

extern void switch_tasks_asm(uint32_t ip, uint32_t kernel_sp, uint32_t user_sp,
                             uint32_t user_bp, struct registers *regs);

extern void switch_tasks_kernel(struct registers *regs, uint32_t ip);
extern void save_registers(struct registers *regs) __attribute__((cdecl));
extern void load_registers(struct registers *regs) __attribute__((cdecl));

void schedule();

void enable_scheduler();
void disable_scheduler();
bool is_scheduler_enabled();

#endif
