#include "sleep.h"
#include <container_of.h>
#include <drivers/character/terminal.h>
#include <elf.h>
#include <fcntl.h>
#include <gdt.h>
#include <interrupts/pic.h>
#include <kernel/addr_space.h>
#include <kernel/paging.h>
#include <kernel/syscall.h>
#include <kmalloc.h>
#include <lib/assert.h>
#include <lib/list.h>
#include <lib/log.h>
#include <lib/refcount.h>
#include <malloc_internal.h>
#include <memory/get_free_pages.h>
#include <panic.h>
#include <scheduler.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <task_queue.h>
#include <types.h>
#include <unistd.h>
#include <utils.h>
#include <vfs.h>

#define SCHED_QUEUE_INIT(name)                                                 \
    ((struct sched_queue){.queue = TASK_QUEUE_INIT((name).queue),              \
                          .current_task = NULL})

struct sched_queue scheduler_queue = SCHED_QUEUE_INIT(scheduler_queue);

void scheduler_init()
{
}

#define SCHED_QUEUE_EMPTY(q) (q.current_task->task == NULL)

static inline void sched_queue_init(struct sched_queue *queue)
{
    *queue = SCHED_QUEUE_INIT(*queue);
}

bool scheduler_enabled = false;

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

struct new_task_usermode_start_args
{
    int argc;
    char **argv;
    ptr_t addr;
};
void new_task_usermode_start(struct new_task_usermode_start_args *args)
{

    struct registers2 *temp_regs = malloc(sizeof(struct registers2));

    temp_regs->esp = 0xBFFEF000;
    scheduler_set_regs_argv(args->argc, args->argv, temp_regs);

    jump_usermode_asm(args->addr, temp_regs, (ptr_t)temp_regs->esp);
}

void user_thread_kernel_start(ptr_t addr)
{

    struct registers2 *temp_regs = malloc(sizeof(struct registers2));
    /* might be stupid */
    temp_regs->esp = (ptr_t)kmalloc(PAGE_SIZE, KMALLOC_USER) + PAGE_SIZE - 0xC;

    jump_usermode_asm(addr, temp_regs, (ptr_t)temp_regs->esp);
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
        memcpy(args + string_offset, argv[i], strlen(argv[i]) + 1);
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
        file_get(file);
        fdlist_set(dest, index, file);
    }
}

/*
 * copies the inital file descriptors from the current task to @new_task
 */
static void copy_init_file_descriptors(struct task *new_task)
{
    /* TODO: maybe have a function to index into the fdlist, like dup2() */
    /* TODO: maybe make stdin/out not readable/writeable in the kernel */

    const char *term_path = "/dev/terminal";
    const char *serial_path = "/dev/serial0";
    const char *keyboard_path = "/dev/keyboard";
    struct task *current_task = get_current_task();

    /* check if the current task is not the inital kernel task, as if it
     * is the fdlist is invalid */
    if (current_task->start_addr != (void *)0xDEADBEEF)
    {
        struct fdlist *current_fds = current_task->descriptors;
        struct fdlist *out_fds = new_task->descriptors;

        fdlist_copy_device(out_fds, current_fds, 0, keyboard_path);
        fdlist_copy_device(out_fds, current_fds, 1, term_path);
        fdlist_copy_device(out_fds, current_fds, 2, term_path);
        fdlist_copy_device(out_fds, current_fds, 3, serial_path);
    }
    else
    {
        fdlist_set(new_task->descriptors, 0, open_device(keyboard_path));
        fdlist_set(new_task->descriptors, 1, open_device(term_path));
        fdlist_set(new_task->descriptors, 2, open_device(term_path));
        fdlist_set(new_task->descriptors, 3, open_device(serial_path));
    }
}

struct task *user_task_alloc(ptr_t ip, ptr_t sp, int argc, char **argv)
{
    struct task *out = task_alloc(ip);

    /* out->stack = get_free_pages(1) + (PAGE_SIZE - 0x30); */

    *(ptr_t *)(out->stack) = (ptr_t)new_task_usermode_start;

    out->addr_space = addr_space_alloc();

    out->ran_setup = false;
    out->regs.esp = (ptr_t)out->stack;

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

    /* stupid */
    struct new_task_usermode_start_args *task_args =
        malloc(sizeof(struct new_task_usermode_start_args));
    task_args->addr = ip;
    task_args->argc = argc;
    task_args->argv = args;
    out->inital_arg = (ptr_t)task_args;
    out->ran_setup = false;

    load_pgdir(temp);

    out->working_dir = get_current_task()->working_dir;

    copy_init_file_descriptors(out);

    return out;
}

struct task *user_thread_alloc(ptr_t ip)
{

    struct addr_space current_addr_space = get_current_task()->addr_space;
    ASSERT(current_addr_space.dir != NULL,
           "null pagedir for current task, this might be because a kernel "
           "thread called this");

    struct task *out = task_alloc(ip);

    out->stack = get_free_pages(1) + (PAGE_SIZE - 0x60);

    out->regs.esp = (ptr_t)out->stack;

    *((ptr_t *)(out->stack)) = (ptr_t)user_thread_kernel_start;
    out->ran_setup = false;
    out->inital_arg = ip;

    out->addr_space = current_addr_space;

    addr_space_load(&out->addr_space);
    struct fdlist *parent_fdlist = get_current_task()->descriptors;
    out->descriptors = parent_fdlist;

    out->name = get_current_task()->name;

    /* TODO: this should duplicate file descriptors */
    copy_init_file_descriptors(out);

    return out;
}

/* TODO: */
SYSCALL_DEFINE1(SYS_clone, int, ptr_t, ip)
{
    add_task(user_thread_alloc(ip));

    return 0;
}

void ktask_run(void (*task)(void *), void *args)
{
    struct task *t = task_args_alloc((ptr_t)task, args);
    add_task(t);
}

void ktask_return()
{

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

struct task *task_args_alloc(ptr_t ip, void *args)
{

    struct task *out = task_alloc(ip);
    *((ptr_t *)(out->stack - sizeof(ptr_t) * 4)) = (ptr_t)ktask_return;
    *((ptr_t *)(out->stack + sizeof(ptr_t))) = (ptr_t)ktask_return;
    return out;
}

struct task *task_alloc(ptr_t ip)
{
    struct task *out = malloc(sizeof(struct task));
    out->name = NULL;
    out->preempt_count = 0;

    out->descriptors = new_fdlist();

    /* TODO: this is really stupid and runs out of space all the time,
     * however it works */
    out->stack = get_free_pages(1) + (PAGE_SIZE - sizeof(ptr_t) * 5);
    out->working_dir = NULL;

    out->regs = (struct registers){};
    out->regs.esp = (ptr_t)out->stack;
    /* out->regs.eflags = 1 << 9; */

    *(ptr_t *)(out->stack) = ip;
    *(ptr_t *)(out->stack + 8) = ip;
    out->addr_space = (struct addr_space){.dir = NULL, .list_head = NULL};

    out->start_addr = (void *)ip;
    out->list = LIST_INIT();
    out->preempt_count = 0;

    out->can_sleep = true;
    out->ran_setup = true;

    return out;
}

struct task *create_kernel_task()
{
    struct task *out = task_alloc(0);
    out->ran_setup = true;

    /* TODO: this needs to be here, but results in code duplication in
     * switch_to() and save_registers() */
    struct registers2 regs;
    regs.fsave_space = malloc(FPU_STATE_SIZE);
    save_registers(&regs);
    out->addr_space = (struct addr_space){.dir = &init_pgdir, NULL};
    out->stack = (void *)regs.esp;
    out->start_addr = (void *)0xDEADBEEF;

    return out;
}

bool init_task_exists()
{
    struct list *pos;
    list_for_each(pos, &scheduler_queue.queue.head)
    {
        struct task *t = TASK_QUEUE_GET_TASK(pos);
        if (t->start_addr == (void *)0xDEADBEEF)
        {
            return true;
        }
    }
    return false;
}

static void remove_task_sched_queue(struct task *task)
{

    if ((ptr_t)task->start_addr == 0xDEADBEEF)
    {
        /* this not panicking might cause problems, but when nothing else is
         * running (so only the init task), and an interrupt happens that will
         * lock a mutex or wait on a queue, the initial task would be removed */
        /* PANIC("attempt to remove initial task"); */
        return;
    }

    irq_state_t state = irq_save_disable();
    if (!init_task_exists())
    {
        PANIC("INIT TASK IS GONE!! in remove_task()\n");
    }
    task_queue_remove(&scheduler_queue.queue, task);

    /* printf("this is the  sched queue head:%p\n",
     * scheduler_queue.head);
     */

    if (task == get_current_task())
    {
        struct list *list = scheduler_queue.queue.head.next;
        scheduler_queue.current_task =
            containerof(list, struct task_queue_entry, list);

        /* scheduler_queue.queue.head = TASK_QUEUE_GET_TASK(list)->list;
         */
        if (list == &scheduler_queue.queue.head)
        {
            PANIC("task queue might be empty!");
            if (scheduler_queue.current_task == NULL)
            {
                PANIC("why is this null:%p\n", scheduler_queue.queue.head);
            }
        }
    }

    if (!init_task_exists())
    {
        PANIC("INIT TASK IS GONE!! in remove_task() second one\n");
    }
}

static void remove_task_load(struct task *old_task)
{
    struct task *current = SCHED_QUEUE_CURRENT_TASK(scheduler_queue);
    if (current->addr_space.dir != NULL)
    {
        addr_space_load(&current->addr_space);
    }
    set_kernel_stack((ptr_t)current->stack);

    switch_to(old_task, get_current_task());
}

void remove_task_add_queue(struct task *task, struct task_queue *queue)
{
    preempt_disable();

    remove_task_sched_queue(task);

    task_queue_push(queue, task);
    remove_task_load(task);

    preempt_enable();
}

void remove_task(struct task *task)
{
    preempt_disable();

    /* save_registers(&task->regs); */
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
        if (scheduler_queue.current_task == NULL)
        {
            struct task_queue_entry *ent = task_queue_entry_alloc(task);

            list_push(&ent->list, &scheduler_queue.queue.head);

            scheduler_queue.current_task = ent;
        }

        task_queue_push(&scheduler_queue.queue, task);
    })
}

int num_task_switches = 0;
void switch_tasks()
{
    struct task_queue_entry *ent = scheduler_queue.current_task;
    struct list *temp = ent->list.next;

    if (temp == &scheduler_queue.queue.head)
    {
        temp = scheduler_queue.queue.head.next;
    }
    struct task_queue_entry *new_task =
        containerof(temp, struct task_queue_entry, list);

    scheduler_queue.current_task = new_task;
    /* current_task = new_current; */
}

void schedule_impl()
{
    /* preempt_disable(); */
    irq_disable();

    increment_sleep_queue();

    if (!scheduler_enabled || scheduler_queue.current_task->task == NULL)
    {
        return;
    }

    struct task *current = SCHED_QUEUE_CURRENT_TASK(scheduler_queue);
    struct task_queue_entry *ent = scheduler_queue.current_task;
    struct task *prev_task = current;

    switch_tasks();

    if (SCHED_QUEUE_EMPTY(scheduler_queue))
    {
        return;
    }
    current = SCHED_QUEUE_CURRENT_TASK(scheduler_queue);
    if (SCHED_QUEUE_CURRENT_TASK(scheduler_queue)->addr_space.dir != NULL)
    {
        addr_space_load(&SCHED_QUEUE_CURRENT_TASK(scheduler_queue)->addr_space);
    }
    set_kernel_stack((ptr_t)SCHED_QUEUE_CURRENT_TASK(scheduler_queue)->stack);

    switch_to(prev_task, SCHED_QUEUE_CURRENT_TASK(scheduler_queue));
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
    if (scheduler_queue.current_task == NULL)
    {
        return NULL;
    }
    return SCHED_QUEUE_CURRENT_TASK(scheduler_queue);
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
    struct list *pos;
    list_for_each(pos, &scheduler_queue.queue.head)
    {
        struct task *t = TASK_QUEUE_GET_TASK(pos);
        printf("%p ", t->start_addr);
    }
    printf("\n");
}

void preempt_disable()
{
    struct task *task = get_current_task();
    if (task == NULL)
    {
        return;
    }

    task->preempt_count--;
    if (task->preempt_count <= 0)
    {
        irq_disable();
    }
    task->preempt_count++;
}
void preempt_enable()
{
    struct task *task = get_current_task();
    if (task == NULL)
    {
        return;
    }

    task->preempt_count--;
    if (task->preempt_count <= 0)
    {
        irq_enable();
    }
}
