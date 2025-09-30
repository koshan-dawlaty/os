#include "elf.h"
#include "gdt.h"
#include "interrupts/pic.h"
#include "kernel/usermode.h"
#include "vfs.h"
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

struct task *new_task(ptr ip, ptr sp)
{
    struct task *out = malloc(sizeof(struct task));

    out->instruction_pointer = ip;
    out->stack_pointer = sp;
    out->bp = sp;

    out->next = NULL;
    out->stack = get_free_pages(1) + (PAGE_SIZE - 0x18);

    out->regs = (struct registers){
        .esp = (ptr)out->stack,
        .ebp = (ptr)out->stack,
        .eflags = 1 << 9,
    };

    /* out->working_dir = get_root_dir(); */
    /* NULL is used to signify root dir */
    out->working_dir = NULL;

    *(ptr *)(out->stack) = (ptr)new_task_usermode_start;
    *(ptr *)(out->stack + 8) = ip;

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

void remove_task(struct task *task)
{
    asm("cli");

    if (task == first_task)
    {
        struct task *temp = first_task->next;
        first_task = temp;

        return;
    }

    struct task *current = first_task;
    struct task *previous = first_task;

    while (current != NULL && current != task)
    {
        previous = current;
        current = current->next;
    }

    if (current == NULL)
    {
        /* task not in queue */
        printf("task not in scheduler queue\n");
        return;
    }

    previous->next = current->next;

    asm("sti");
}

void add_task(struct task *task)
{
    printf("adding task %p\n", task);
    while (1)
    {
    };

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

void add_task_queue(struct task **queue, struct task *task)
{

    if (*queue == NULL)
    {
        *queue = task;
        return;
    }

    struct task *temp = *queue;
    *queue = task;
    (*queue)->next = temp;
}

void remove_task_queue(struct task **queue, struct task *task)
{

    struct task *current = *queue;
    struct task *previous = *queue;
    while (current != NULL && current != task)
    {
        previous = current;
        current = current->next;
    }

    if (current == NULL)
    {
        printf("did not find task in queue\n");
        /* task not in queue */
        return;
    }

    previous->next = current->next;
}

int num_task_switches = 0;
void switch_tasks()
{
    /* printf("switching from:%p ", current_task); */

    if (current_task->next != NULL)
    {
        current_task = current_task->next;
    }
    else
    {
        current_task = first_task;
    }
    /* printf("to:%p\n", current_task); */
}

void schedule()
{
    if (!scheduler_enabled || current_task == NULL)
    {
        return;
    }

    save_registers(&current_task->regs);

    switch_tasks();

    if (current_task == NULL)
    {
        return;
    }

    load_pgdir(current_task->pgdir);
    set_kernel_stack((uint32_t)current_task->stack);

    PIC_sendEOI(0);
    load_registers(&current_task->regs);
}

struct task *get_current_task()
{
    return current_task;
}

void free_task(struct task *t)
{
    /* TODO: free the pagedir */
    free(t);
}
