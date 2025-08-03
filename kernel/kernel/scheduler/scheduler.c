#include "elf.h"
#include "gdt.h"
#include "interrupts/pic.h"
#include "kernel/usermode.h"
#include "kernel/utils.h"
#include "panic.h"
#include "unistd.h"
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

struct task *new_task(uint32_t ip, uint32_t sp)
{
    struct task *out = malloc(sizeof(struct task));

    out->instruction_pointer = ip;
    out->stack_pointer = sp;
    /* TODO: might be bad */
    out->bp = sp;
    /* user task */
    out->regs =
        (struct registers){.esp = 0xBFFEF000, .ebp = 0xBFFEF000, .frame = {}};
    /* kernel task */
    // out->regs = (struct registers){};

    out->next = NULL;

    out->stack = get_free_pages(1) + PAGE_SIZE;

    out->pgdir = new_pagedir();

    write_kernel_page_tables(out->pgdir);

    return out;
}

void add_task(struct task *task)
{

    /* asm("cli"); */

    if (current_task == NULL)
    {
        current_task = task;
        first_task = task;
        return;
        /* goto end; */
    }

    struct task *temp = first_task;
    first_task = task;
    task->next = temp;
    /* current_task->next = task; */

    /* current_task = task; */

    /* end: */
    /*     printf("jumping to:%p\n", current_task->instruction_pointer); */

    /*     jump_usermode_asm(current_task->instruction_pointer); */
}

int num_task_switches = 0;
void switch_tasks(struct registers regs)
{

    uint32_t ip = regs.frame.ip;
    uint32_t sp = regs.frame.sp;
    uint32_t bp = regs.ebp;

    if (!scheduler_enabled)
    {
        return;
    }

    /* printf("task switch number:%d\n", num_task_switches); */
    num_task_switches++;

    /* asm("cli"); */
    /* asm("sti"); */

    if (current_task == NULL)
    {
        return;
    }

    /* printf("current ip:%p, new ip:%p\n", current_task->instruction_pointer,
     * ip); */

    /* TODO: this is very bad */
    /* if (ip < current_task->instruction_pointer + 0x1000 && */
    /*     ip > current_task->instruction_pointer - 0x1000) */
    /* { */
    current_task->instruction_pointer = ip;
    current_task->stack_pointer = sp;
    current_task->bp = bp;
    /* printf("task registers addr:%p, registers addr:%p\n", */
    /*        &current_task->regs, &regs); */

    memcpy(&current_task->regs, &regs, sizeof(struct registers));

    /* current_task->regs.eax = 0x12345678; */
    /* } */
    /* else */
    /* { */
    /*     printf("bad\n"); */
    /*     return; */
    /* } */

    /* printf("switching from task %p", current_task); */
    if (current_task->next != NULL)
    {
        current_task = current_task->next;
    }
    else
    {
        current_task = first_task;
    }
    /* printf(" to task %p\n", current_task); */

    /* print_regsiters(current_task->regs); */

    /*
printf("switching to:%p\n", current_task->pgdir);
printf("kernel stack:%p\n", current_task->stack);
printf("user stack:%p\n", current_task->stack_pointer);
printf("jumping to %p\n", current_task->instruction_pointer);
    */
    load_pgdir(current_task->pgdir);

    bool going_to_usermode = current_task->instruction_pointer < 0xC0000000;

    /* bool going_to_usermode = (current_task->regs.frame.cs & 3) == 3; */

    PIC_sendEOI(0);

    if (going_to_usermode)
    {
        /* printf("user\n"); */
        uint32_t stack = current_task->regs.esp;

        /* if there was not a context switch for the irq the interrupt frame
         * stack is invalid */
        if ((current_task->regs.frame.cs & 3) == 3)
        {
            stack = current_task->regs.frame.sp;
        }
        /* printf("user stack:%p\n", stack); */
        switch_tasks_asm(current_task->instruction_pointer,
                         (uint32_t)current_task->stack, stack, current_task->bp,
                         &current_task->regs);
    }
    else
    {
        printf("kernel\n");
        printf("registers bp:%p\n", current_task->regs.ebp);

        switch_tasks_kernel(&current_task->regs,
                            current_task->instruction_pointer);
    }
}
