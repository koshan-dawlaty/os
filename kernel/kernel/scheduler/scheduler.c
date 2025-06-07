#include "elf.h"
#include "kernel/usermode.h"
#include <kernel/paging.h>
#include <scheduler.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct task *current_task = NULL;
struct task *first_task = NULL;

struct task *new_task(uint32_t ip)
{
    struct task *out = malloc(sizeof(struct task));

    out->instruction_pointer = ip;
    out->next = NULL;

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
        /* goto end; */
    }

    if (current_task->next != NULL)
    {
        current_task->next = first_task;
    }

    task->next = current_task->next;
    current_task->next = task;

    /* current_task = task; */

    /* end: */
    /*     printf("jumping to:%p\n", current_task->instruction_pointer); */

    /*     jump_usermode_asm(current_task->instruction_pointer); */
}

void switch_tasks(uint32_t ip)
{
    /* printf("switch tasks\n"); */

    asm("cli");
    /* asm("sti"); */

    if (current_task == NULL)
    {
        return;
    }

    printf("current ip:%p, new ip:%p\n", current_task->instruction_pointer, ip);

    /* current_task->instruction_pointer = ip; */
    /* TODO: this is very bad */
    if (ip < current_task->instruction_pointer + 0x1000 &&
        ip > current_task->instruction_pointer - 0x1000)
    {
        current_task->instruction_pointer = ip;
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

    printf("ip:%p\n", current_task->instruction_pointer);

    load_pgdir(current_task->pgdir);

    /* asm("sti"); */
    jump_usermode_asm(current_task->instruction_pointer);

    /* MAY NOT WORK */
    /* asm volatile("jmp *%0" : : "r"(current_task->instruction_pointer)); */
}
