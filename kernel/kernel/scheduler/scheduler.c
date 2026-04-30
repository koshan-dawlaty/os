#include "kernel/syscall.h"
#include "kmalloc.h"
#include "lib/log.h"
#include "sleep.h"
#include "sys/syscall.h"
#include "unistd.h"
#include "utils.h"
#include "vfs.h"
#include <container_of.h>
#include <drivers/character/terminal.h>
#include <elf.h>
#include <fcntl.h>
#include <gdt.h>
#include <interrupts/pic.h>
#include <kernel/addr_space.h>
#include <kernel/paging.h>
#include <kernel/types.h>
#include <lib/assert.h>
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
#include <task_queue.h>

TASK_QUEUE_DEFINE(scheduler_queue);

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

void new_task_usermode_start(int argc, char **argv, ptr addr)
{

    struct registers *temp_regs = malloc(sizeof(struct registers));
    *temp_regs = (struct registers){0};

    temp_regs->esp = 0xBFFEF000;
    scheduler_set_regs_argv(argc, argv, temp_regs);

    jump_usermode_asm(addr, temp_regs, (ptr)temp_regs->esp);
}

void user_thread_kernel_start(ptr addr)
{

    logd("user thread:%p", addr);

    struct registers *temp_regs = malloc(sizeof(struct registers));
    *temp_regs = (struct registers){0};

    temp_regs->esp = (ptr)kmalloc(PAGE_SIZE, KMALLOC_USER) + PAGE_SIZE - 0xC;

    jump_usermode_asm(addr, temp_regs, (ptr)temp_regs->esp);
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

/*
 * attempts to copy file at @index in @src to @index in @dest
 * if the file at @index does not exist, @dev_path is created as a device, and
 * put in @dest at @index
 *
 * used to copy device file descriptors to a new process
 */
static void fdlist_copy_device(struct fdlist *dest, struct fdlist *src,
                               size_t index, const char *dev_path)
{
    struct file *file = fdlist_get(src, index);
    if (file == NULL)
    {
        fdlist_set(dest, index, open_device(dev_path));
    }
    else
    {
        fdlist_set(dest, index, file);
    }
}

struct task *new_task_user(ptr ip, ptr sp, int argc, char **argv)
{
    struct task *out = new_task(ip);
    out->preempt_count = 0;

    out->list = LIST2C_INIT();

    out->start_addr = (void *)ip;

    out->bp = sp;

    out->stack = get_free_pages(1) + (PAGE_SIZE - 0x30);
    out->descriptors = new_fdlist();

    out->regs = (struct registers){
        .esp = (ptr)out->stack,
        .ebp = (ptr)out->stack,
        .eflags = 1 << 9,
#ifdef CONFIG_x86_64
        .edi = (ip),
#endif
    };

    /* out->working_dir = get_root_dir(); */
    /* NULL is used to signify root dir */
    out->working_dir = NULL;

    *(ptr *)(out->stack) = (ptr)new_task_usermode_start;
    /* edx is the third argument first and second are set later in this
     * function
     */
    /* #ifdef CONFIG_x86_32 */
    /*     *(ptr *)(out->stack + sizeof(ptr) * 2) = ip; */
    /* #endif */

    out->addr_space = addr_space_alloc();

    char **args_temp = malloc(argc * sizeof(char *));
    for (int i = 0; i < argc; i++)
    {
        args_temp[i] = malloc(strlen(argv[i]) + 1);
        memcpy(args_temp[i], argv[i], strlen(argv[i]) + 1);
        args_temp[i][strlen(argv[i])] = 0;
    }

    /* TODO: this calls load_pgdir alot */
    struct pagedir *temp = get_current_pagedir();
    addr_space_load(&out->addr_space);
    char **args = create_user_args(argc, args_temp);

    scheduler_set_regs_arguments(argc, args, ip, &out->regs);

    load_pgdir(temp);

    const char *term_path = "/terminal";
    const char *serial_path = "/serial0";
    const char *keyboard_path = "/keyboard";
    /* TODO: maybe have a function to index into the fdlist, like dup2() */
    /* TODO: maybe make stdin/out not readable/writeable in the kernel */

    struct task *current_task = get_current_task();

    /* check if the current task is not the inital kernel task, as if it is the
     * fdlist is invalid */
    if (current_task->start_addr != (void *)0xDEADBEEF)
    {
        struct fdlist *current_fds = current_task->descriptors;
        struct fdlist *out_fds = out->descriptors;

        fdlist_copy_device(out_fds, current_fds, 0, keyboard_path);
        fdlist_copy_device(out_fds, current_fds, 1, term_path);
        fdlist_copy_device(out_fds, current_fds, 2, term_path);
        fdlist_copy_device(out_fds, current_fds, 3, serial_path);
    }
    else
    {
        fdlist_set(out->descriptors, 0, open_device(keyboard_path));
        fdlist_set(out->descriptors, 1, open_device(term_path));
        fdlist_set(out->descriptors, 2, open_device(term_path));
        fdlist_set(out->descriptors, 3, open_device(serial_path));
    }

    return out;
}

struct task *user_thread_alloc(ptr ip)
{

    struct addr_space current_addr_space = current_task->addr_space;
    ASSERT(current_addr_space.dir != NULL,
           "null pagedir for current task, this might be because a kernel "
           "thread called this");

    struct task *out = new_task(ip);
    out->preempt_count = 0;
    out->list = LIST2C_INIT();

    out->start_addr = (void *)ip;

    out->stack = get_free_pages(1) + (PAGE_SIZE - 0x60);
    out->bp = (ptr)out->stack;
    out->descriptors = new_fdlist();

    out->regs = (struct registers){
        .esp = (ptr)out->stack,
        .ebp = (ptr)out->stack,
        .eflags = 1 << 9,
#ifdef CONFIG_x86_64
        .edi = (ip),
#endif
    };

    /* out->working_dir = get_root_dir(); */
    /* NULL is used to signify root dir */
    out->working_dir = NULL;

    *((ptr *)(out->stack)) = (ptr)user_thread_kernel_start;
    /* edx is the third argument first and second are set later in this
     * function
     */
    *(ptr *)(out->stack + sizeof(ptr)) = ip;
#ifdef CONFIG_x86_32
    *(ptr *)(out->stack + sizeof(ptr) * 2) = ip;
#endif

    out->addr_space = current_addr_space;

    addr_space_load(&out->addr_space);
    struct fdlist *parent_fdlist = get_current_task()->descriptors;
    out->descriptors = parent_fdlist;

    /* TODO: this should duplicate file descriptors */
    const char *term_path = "/terminal";
    const char *keyboard_path = "/keyboard";
    const char *serial_path = "/serial0";
    /* TODO: maybe have a function to index into the fdlist, like dup2() */
    /* TODO: maybe make stdin/out not readable/writeable in the kernel */
    /* fdlist_add(out->descriptors, open_device(keyboard_path)); /1* 0, stdin
     * *1/ */
    /* fdlist_add(out->descriptors, open_device(term_path));     /1* 1, stdout
     * *1/ */
    /* fdlist_add(out->descriptors, open_device(term_path));     /1* 2, stderr
     * *1/ */

    /* fdlist_set(out->descriptors, 0, open_device(keyboard_path)); */
    /* fdlist_set(out->descriptors, 1, open_device(term_path)); */
    /* fdlist_set(out->descriptors, 2, open_device(term_path)); */

    if (current_task->start_addr != (void *)0xDEADBEEF)
    {
        struct fdlist *current_fds = current_task->descriptors;
        struct fdlist *out_fds = out->descriptors;

        /* FIXME: the open_device calls get evaluated */
        fdlist_copy_device(out_fds, current_fds, 0, keyboard_path);
        fdlist_copy_device(out_fds, current_fds, 1, term_path);
        fdlist_copy_device(out_fds, current_fds, 2, term_path);
        fdlist_copy_device(out_fds, current_fds, 3, serial_path);
    }
    else
    {
        fdlist_set(out->descriptors, 0, open_device(keyboard_path));
        fdlist_set(out->descriptors, 1, open_device(term_path));
        fdlist_set(out->descriptors, 2, open_device(term_path));
        fdlist_set(out->descriptors, 3, open_device(serial_path));
    }

    return out;
}

/* TODO: */
SYSCALL_DEFINE1(SYS_clone, int, ptr, ip)
{
    add_task(user_thread_alloc(ip));

    return 0;
}

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
    out->preempt_count = 0;

    out->descriptors = new_fdlist();

    /* TODO: this is really stupid and runs out of space all the time,
     * however it works */
    out->stack = get_free_pages(1) + (PAGE_SIZE - sizeof(ptr) * 5);
    out->working_dir = NULL;

    out->regs.esp = (ptr)out->stack;
    out->regs.eflags = 1 << 9;

    *(ptr *)(out->stack) = ip;
    out->addr_space = (struct addr_space){.dir = NULL, .list_head = NULL};

    out->start_addr = (void *)ip;
    out->list = LIST2C_INIT();
    out->preempt_count = 0;

    out->can_sleep = false;

    return out;
}

struct task *create_kernel_task()
{
    struct task *out = new_task(0);

    save_registers(&out->regs);
    out->addr_space = (struct addr_space){.dir = &init_pgdir, NULL};
    out->stack = (void *)out->regs.esp;
    out->start_addr = (void *)0xDEADBEEF;

    return out;
}

bool init_task_exists()
{
    struct list2c *pos;
    list2c_for_each(pos, &scheduler_queue.head)
    {
        struct task *t = containerof(pos, struct task, list);
        if (t->start_addr == (void *)0xDEADBEEF)
        {
            return true;
        }
    }
    return false;
}

static void remove_task_sched_queue(struct task *task)
{

    if ((ptr)task->start_addr == 0xDEADBEEF)
    {
        panic("attempt to remove initial task");
    }

    irq_state_t state = irq_save_disable();
    if (!init_task_exists())
    {
        printf("INIT TASK IS GONE!! in remove_task()\n");
        while (1)
        {
            irq_wait();
        }
    }
    /* save_registers(&current_task->regs); */
    task_queue_remove(&scheduler_queue, task);

    /* printf("this is the  sched queue head:%p\n", scheduler_queue.head);
     */

    if (task == current_task)
    {
        current_task =
            containerof(scheduler_queue.head.next, struct task, list);
        if (&current_task->list == &scheduler_queue.head)
        {
            panic("task queue might be empty!");
            if (current_task == NULL)
            {
                printf("why is this null:%p\n", scheduler_queue.head);
            }
        }
    }

    if (!init_task_exists())
    {
        printf("INIT TASK IS GONE!! in remove_task() second one\n");
    }
}

static void remove_task_load(struct task *task)
{
    if (current_task->addr_space.dir != NULL)
    {
        addr_space_load(&current_task->addr_space);
    }
    set_kernel_stack((ptr)current_task->stack);

    load_registers(&current_task->regs);
}

void remove_task_add_queue(struct task *task, struct task_queue *queue)
{
    preempt_disable();

    remove_task_sched_queue(task);
    task->list.next = NULL;
    task_queue_push(queue, task);
    remove_task_load(task);

    preempt_enable();
}

void remove_task(struct task *task)
{
    preempt_disable();

    remove_task_sched_queue(task);
    remove_task_load(task);

    preempt_enable();
}

void task_remove_free(struct task *task)
{
    preempt_disable();
    remove_task_sched_queue(task);
    free_task(task);
    remove_task_load(task);
    preempt_enable();
}

void add_task(struct task *task)
{
    IRQ_OFF({
        if (current_task == NULL)
        {
            current_task = task;
        }

        task_queue_push(&scheduler_queue, task);
    })
}

int num_task_switches = 0;
void switch_tasks()
{
    struct list2c *temp = current_task->list.next;

    if (temp == &scheduler_queue.head)
    {
        temp = scheduler_queue.head.next;
    }
    /*     if (temp == NULL) */
    /*     { */
    /*         temp = &scheduler_queue.head; */
    /*     } */

    struct task *new_current = containerof(temp, struct task, list);
    /*
printf("START\n");
printf("%p\n", new_current);
printf("%p\n", &new_current->list);
printf("shouldnt be true:%d\n",
       &new_current->list == &scheduler_queue.head);
printf("%p\n", new_current->list.next);
printf("%p\n", new_current->list.next->next);
printf("start_addr:%p\n", new_current->start_addr);
    */

    current_task = new_current;
}

void schedule_impl()
{
    /* preempt_disable(); */
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

    if (current_task->addr_space.dir != NULL)
    {
        addr_space_load(&current_task->addr_space);
    }

    set_kernel_stack((ptr)current_task->stack);

    /* PIC_sendEOI(0); */

    load_registers(&current_task->regs);
}

void schedule()
{

    /* preempt_disable(); */
    irq_disable();
    schedule_impl();
    irq_enable();
}

struct task *get_current_task()
{
    return current_task;
}

void free_task(struct task *t)
{
    addr_space_free(t->addr_space);
    /* TODO: free the pagedir */
    fdlist_free(t->descriptors);

    free(t);
}

void print_tasks()
{
    struct list2c *pos;
    list2c_for_each(pos, &scheduler_queue.head)
    {
        struct task *t = containerof(pos, struct task, list);
        printf("%p ", t->start_addr);
    }
    printf("\n");
}
