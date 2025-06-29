#include "elf.h"
#include "gdt.h"
#include "interrupts/pic.h"
#include "kernel/usermode.h"
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
    out->regs = (struct registers){};

    out->next = NULL;
    out->stack = malloc(4096);
    *(char *)out->stack = 'a';

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

void switch_tasks(struct registers regs)
{

    uint32_t ip = regs.frame.ip;
    uint32_t sp = regs.frame.sp;
    uint32_t bp = regs.ebp;

    printf("test\n");
    if (!scheduler_enabled)
    {
        return;
    }

    /* asm("cli"); */
    /* asm("sti"); */

    if (current_task == NULL)
    {
        return;
    }

    /* printf("current ip:%p, new ip:%p\n", current_task->instruction_pointer,
     * ip); */

    /* TODO: this is very bad */
    if (ip < current_task->instruction_pointer + 0x1000 &&
        ip > current_task->instruction_pointer - 0x1000)
    {
        current_task->instruction_pointer = ip;
        current_task->stack_pointer = sp;
        current_task->bp = bp;
        memcpy(&current_task->regs, &regs, sizeof(struct registers));
    }
    else
    {
        printf("bad\n");
        return;
    }

    if (current_task->next != NULL)
    {
        current_task = current_task->next;
    }
    else
    {
        /* printf("no more tasks in queue\n"); */
        current_task = first_task;
    }

    printf("switching to:%p\n", current_task->pgdir);
    printf("kernel stack:%p\n", current_task->stack);
    printf("user stack:%p\n", current_task->stack_pointer);
    printf("jumping to %p\n", current_task->instruction_pointer);

    /* set_kernel_stack((uint32_t)current_task->stack); */
    PIC_sendEOI(0);
    load_pgdir(current_task->pgdir);

    switch_tasks_asm(current_task->instruction_pointer,
                     (uint32_t)current_task->stack, current_task->stack_pointer,
                     current_task->bp);
    /* jump_usermode_asm(current_task->instruction_pointer); */
}
