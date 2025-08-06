#include "elf.h"
#include "gdt.h"
#include "interrupts/pic.h"
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

void test2()
{
    printf("test\n");
    *(char *)(0x12345) = 'a';
}

struct task *new_task(uint32_t ip, uint32_t sp)
{
    struct task *out = malloc(sizeof(struct task));

    out->instruction_pointer = ip;
    out->stack_pointer = sp;
    /* TODO: might be bad */
    out->bp = sp;
    /* user task */
    /* kernel task */
    // out->regs = (struct registers){};

    out->next = NULL;
    out->stack = get_free_pages(1) + (PAGE_SIZE - 4);
    printf("stack:%p\n", out->stack);

    out->regs = (struct registers){.esp = (uint32_t)out->stack,
                                   .ebp = (uint32_t)out->stack,
                                   .eflags = 1 << 9,
                                   .frame = {}};

    /* *(uint32_t *)(out->stack) = (uint32_t)0xc0105655; */
    *(uint32_t *)(out->stack) = (uint32_t)ip;

    out->pgdir = new_pagedir();
    write_kernel_page_tables(out->pgdir);

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
    printf("switching from task:%p ", &current_task->regs);
    if (current_task->next != NULL)
    {
        current_task = current_task->next;
    }
    else
    {
        current_task = first_task;
    }
    printf("to task:%p\n", &current_task->regs);
}

void schedule()
{
    if (!scheduler_enabled)
    {
        return;
    }

    save_registers(&current_task->regs);
    /* printf("%p\n", current_task->regs.eflags); */
    switch_tasks();

    load_pgdir(current_task->pgdir);
    set_kernel_stack((uint32_t)current_task->stack);

    load_registers(&current_task->regs);

    PIC_sendEOI(0);
}
