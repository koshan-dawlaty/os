#include "sleep.h"
#include "unistd.h"
#include "utils.h"
#include "vfs.h"
#include <assert.h>
#include <drivers/character/terminal.h>
#include <elf.h>
#include <fcntl.h>
#include <gdt.h>
#include <interrupts/pic.h>
#include <kernel/paging.h>
#include <kernel/types.h>
#include <lib/list.h>
#include <memory/get_free_pages.h>
#include <panic.h>
#include <scheduler.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct task_queue scheduler_queue = TASK_QUEUE_INIT();

bool scheduler_enabled = false;
struct task *current_task = NULL;
struct task *first_task = NULL;

extern struct pagedir init_pgdir;

extern void *kernel_task1;
extern void *kernel_task2;
extern void *kernel_task3;

static char *get_task_name(void *func)
{
    if (func == &kernel_task1)
    {
        return "task1";
    }
    else if (func == &kernel_task2)
    {
        return "task2";
    }
    else if (func == &kernel_task3)
    {
        return "task3";
    }

    return "unknown";
}

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

void new_task_usermode_start(int argc, char **argv, ptr addr)
{

    struct registers *temp_regs = malloc(sizeof(struct registers));
    *temp_regs = (struct registers){0};

    temp_regs->esp = 0xBFFEF000;
    scheduler_set_regs_argv(argc, argv, temp_regs);

    jump_usermode_asm(addr, temp_regs);
}

/* copies args to userspace */
char **create_user_args(int argc, char **argv)
{
    /* TODO: this can run out of space */
    char **args = get_free_pages_user(1);

    size_t string_offset = argc * sizeof(char *);
    if (string_offset >= PAGE_SIZE)
    {
        panic("ran out of space in user args");
    }

    for (int i = 0; i < argc; i++)
    {
        memcpy(args + string_offset, argv[i], strlen(argv[i]));
        *(char *)(args + string_offset + strlen(argv[i]) + 1) = 0;

        args[i] = (char *)(args + string_offset);

        string_offset += strlen(argv[i]) + 1;
    }

    return args;
}

struct task *new_task_user(ptr ip, ptr sp, int argc, char **argv)
{
    struct task *out = malloc(sizeof(struct task));

    out->start_addr = (void *)ip;

    out->bp = sp;

    out->stack = get_free_pages(1) + (PAGE_SIZE - 0x30);
    out->descriptors = new_fdlist();

    out->regs = (struct registers){
        .esp = (ptr)out->stack,
        .ebp = (ptr)out->stack,
        .eflags = 1 << 9,
        .edi = (ip),
    };

    /* out->working_dir = get_root_dir(); */
    /* NULL is used to signify root dir */
    out->working_dir = NULL;

    *(ptr *)(out->stack) = (ptr)new_task_usermode_start;
    /* edx is the third argument first and second are set later in this function
     */
    /* *(ptr *)(out->stack + 8) = ip; */

    out->pgdir = new_pagedir();
    write_kernel_page_tables(out->pgdir);

    char **args_temp = malloc(argc * sizeof(char *));
    for (int i = 0; i < argc; i++)
    {
        args_temp[i] = malloc(strlen(argv[i]) + 1);
        memcpy(args_temp[i], argv[i], strlen(argv[i]) + 1);
        args_temp[i][strlen(argv[i])] = 0;
    }

    /* TODO: this calls load_pgdir alot */
    struct pagedir *temp = get_current_pagedir();
    load_pgdir(out->pgdir);
    char **args = create_user_args(argc, args_temp);

    scheduler_set_regs_arguments(argc, args, ip, &out->regs);

    load_pgdir(temp);

    const char *term_path = "/terminal";
    const char *keyboard_path = "/keyboard";
    /* TODO: maybe have a function to index into the fdlist, like dup2() */
    /* TODO: maybe make stdin/out not readable/writeable in the kernel */
    fdlist_add(out->descriptors, open_device(keyboard_path)); /* 0, stdin */
    fdlist_add(out->descriptors, open_device(term_path));     /* 1, stdout */
    fdlist_add(out->descriptors, open_device(term_path));     /* 2, stderr */

    return out;
}

/*
struct task *new_task(ptr ip)
{
    struct task *out = malloc(sizeof(struct task));

    out->next = NULL;

    out->stack = get_free_pages(1) + (PAGE_SIZE - 0x30);
    out->working_dir = NULL;

    out->regs.esp = (ptr)out->stack;
    out->regs.eflags = 1 << 9;

    *(ptr *)(out->stack) = ip;
    out->pgdir = new_pagedir();
    write_kernel_page_tables(out->pgdir);

    return out;
}*/

void ktask_run(void (*task)(void *), void *args)
{
    add_task(new_task_args((ptr)task, args));
}

void ktask_return()
{
    printf("ktask_return\n");
    irq_state_t irq_state = irq_save_disable();
    {
        struct task *current = get_current_task();
        remove_task(current);
        free_task(current);
    }
    irq_restore(irq_state);

    irq_enable();
    schedule();
    /* execution maybe shouldnt get here */
    panic("ktask_return end reached\n");
}

struct task *new_task_args(ptr ip, void *args)
{
    struct task *out = new_task(ip);
    scheduler_set_first_arg(args, &out->regs);
    /* *((ptr *)(out->stack - sizeof(ptr))) = (ptr)ktask_return; */
    /* *((ptr *)(out->stack - sizeof(ptr) * 4)) = 0x123; */
    *((ptr *)(out->stack - sizeof(ptr) * 4)) = (ptr)ktask_return;
    *((ptr *)(out->stack + sizeof(ptr))) = (ptr)ktask_return;
    return out;
}

struct task *new_task(ptr ip)
{
    struct task *out = malloc(sizeof(struct task));

    out->descriptors = new_fdlist();

    /* TODO: this is really stupid and runs out of space all the time, however
     * it works */
    out->stack = get_free_pages(1) + (PAGE_SIZE - sizeof(ptr) * 5);
    out->working_dir = NULL;

    out->regs.esp = (ptr)out->stack;
    out->regs.eflags = 1 << 9;

    *(ptr *)(out->stack) = ip;
    out->pgdir = NULL;

    out->start_addr = (void *)ip;

    return out;
}

struct task *create_kernel_task()
{
    struct task *out = new_task(0);

    save_registers(&out->regs);
    out->pgdir = &init_pgdir;
    out->stack = (void *)out->regs.esp;
    out->start_addr = (void *)0xDEADBEEF;

    return out;
}

void remove_task(struct task *task)
{
    irq_state_t irq_state = irq_save_disable();
    task_queue_remove(&scheduler_queue, task);

    if (task == current_task)
    {
        current_task = LIST_ENTRY(current_task->list.next, struct task, list);
        if (current_task == NULL)
        {
            current_task = LIST_ENTRY(scheduler_queue.head, struct task, list);
        }
    }
    irq_restore(irq_state);
    printf("%p\n", get_current_task()->start_addr);
    load_pgdir(get_current_task()->pgdir);
    load_registers(&get_current_task()->regs);
}

void add_task(struct task *task)
{
    if (current_task == NULL)
    {
        current_task = task;
    }

    IRQ_OFF({ task_queue_push(&scheduler_queue, task); })
}

int num_task_switches = 0;
void switch_tasks()
{
    char *old_name = get_task_name(current_task->start_addr);
    struct list *temp = current_task->list.next;

    if (temp == NULL)
    {
        temp = scheduler_queue.head;
    }

    struct task *new_current = LIST_ENTRY(temp, struct task, list);
    current_task = new_current;
    /* printf("switching from:%s to:%s\n", old_name, */
    /*        get_task_name(current_task->start_addr)); */

    if (new_current->start_addr == &kernel_task2)
    {
        /* printf("task2 in scheduler_queue\n"); */
    }
}

void display_queue()
{
    struct list *pos;
    list_for_each(pos, scheduler_queue.head)
    {
        struct task *t = LIST_ENTRY(pos, struct task, list);
        printf("%s ", get_task_name(t->start_addr));
    }
    printf("\n");
}

void schedule()
{

    /* display_queue(); */
    irq_disable();

    increment_sleep_queue();

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

    /* printf("%p\n", current_task->pgdir->pgdir); */
    /* printf("%p\n", current_task->pgdir->pgdir_virtual); */

    if (current_task->pgdir != NULL)
    {
        load_pgdir(current_task->pgdir);
    }

    set_kernel_stack((ptr)current_task->stack);

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
    fdlist_free(t->descriptors);

    free(t);
}
