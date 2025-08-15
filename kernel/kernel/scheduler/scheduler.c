#include "elf.h"
#include "gdt.h"
#include "interrupts/pic.h"
#include "kernel/usermode.h"
#include <kernel/paging.h>
#include <scheduler.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool scheduler_enabled = false;
struct task *current_task = NULL;
struct task *first_task = NULL;

extern struct pagedir init_pgdir;

void enable_scheduler()
{
    scheduler_enabled = true;
}

void disable_scheduler()
{
    scheduler_enabled = false;
}

bool is_scheduler_enabled()
{
    return scheduler_enabled;
}

void new_task_usermode_start(uint32_t addr)
{

    struct registers *temp_regs = malloc(sizeof(struct registers));
    *temp_regs = (struct registers){0};
    jump_usermode_asm(addr, temp_regs);
}

struct task *new_task(uint32_t ip, uint32_t sp)
{
    struct task *out = malloc(sizeof(struct task));

    out->instruction_pointer = ip;
    out->stack_pointer = sp;
    out->bp = sp;

    out->next = NULL;
    out->stack = get_free_pages(1) + (PAGE_SIZE - 0xC);

    out->regs = (struct registers){
        .esp = (uint32_t)out->stack,
        .ebp = (uint32_t)out->stack,
        .eflags = 1 << 9,
    };

    *(uint32_t *)(out->stack) = (uint32_t)new_task_usermode_start;
    *(uint32_t *)(out->stack + 8) = ip;

    out->pgdir = new_pagedir();
    write_kernel_page_tables(out->pgdir);

    return out;
}

struct task *create_kernel_task()
{
    struct task *out = new_task(0, 0);

    save_registers(&out->regs);
    out->pgdir = &init_pgdir;
    out->stack = (void *)out->regs.esp;

    return out;
}

void add_task(struct task *task)
{

    asm("cli");

    if (current_task == NULL)
    {
        current_task = task;
        first_task = task;
        goto end;
    }

    struct task *temp = first_task;
    first_task = task;
    task->next = temp;

end:
    asm("sti");
}

int num_task_switches = 0;
void switch_tasks()
{
    if (current_task->next != NULL)
    {
        current_task = current_task->next;
    }
    else
    {
        current_task = first_task;
    }
}

void schedule()
{
    if (!scheduler_enabled)
    {
        return;
    }

    save_registers(&current_task->regs);
    switch_tasks();

    load_pgdir(current_task->pgdir);
    set_kernel_stack((uint32_t)current_task->stack);

    PIC_sendEOI(0);
    load_registers(&current_task->regs);
}

struct task *get_current_task()
{
    return current_task;
}
