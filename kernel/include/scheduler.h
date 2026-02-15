#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "kernel/addr_space.h"
#include <compiler.h>
#include <kernel/paging.h>
#include <kernel/types.h>
#include <lib/list.h>
#include <stdbool.h>
#include <stdint.h>
#include <vfs.h>

/* TODO: mabye do the architecture specific part of this architecture
 * independent */
/* the stack pointer is the only member of this struct that is guaranteed to
 * exist */
struct registers
{

#ifdef CONFIG_x86_32
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t eflags;
#elif defined(CONFIG_x86_64)

    /*  0   8   10  18 */
    u64 gs, fs, es, ds;
    /*  20   28   30   38   40   48   50   58 */
    u64 edi, esi, ebp, esp, ebx, edx, ecx, eax;
    /*  60 */
    u64 eflags;
    /*  68  70  78   80   88   90   98   A0 */
    u64 r8, r9, r10, r11, r12, r13, r14, r15;
#endif

} __packed;

struct task
{
    struct list list;

    struct addr_space addr_space;

    void *stack;
    struct registers regs;
    ptr bp;
    struct dentry *working_dir;
    struct fdlist *descriptors;

    void *start_addr; /* used for debugging */
};

void switch_tasks();

/* adds a task to the scheduler task queue */
void add_task(struct task *task);

/* removes a task from the scheduler task queue */
void remove_task(struct task *task);

/* adds task to queue */
void add_task_queue(struct task **queue, struct task *task);
/* removes task from queue */
void remove_task_queue(struct task **queue, struct task *task);

/*
 * initializes the page directory as a free page
 */
struct task *new_task(ptr ip);

struct task *new_task_user(ptr ip, ptr sp, int argc, char **argv);
struct task *user_thread_alloc(ptr ip);

/*
 * frees task t
 * this does not dequeue t
 */
void free_task(struct task *t);
/*
 * creates a task for the currently running kernel code
 * intended to be used to create a task for the code running initially
 */
struct task *create_kernel_task();

extern void switch_tasks_asm(uint32_t ip, uint32_t kernel_sp, uint32_t user_sp,
                             uint32_t user_bp, struct registers *regs);

extern void switch_tasks_kernel(struct registers *regs, uint32_t ip);
extern void save_registers(struct registers *regs);
extern void load_registers(struct registers *regs);

extern void jump_usermode_asm(ptr addr, struct registers *regs, ptr sp);

void schedule();

void enable_scheduler();
void disable_scheduler();
bool is_scheduler_enabled();

struct task *get_current_task();
void scheduler_set_regs_arguments(int argc, char **argv, ptr ip,
                                  struct registers *regs);

void scheduler_set_regs_argv(int argc, char **argv, struct registers *regs);
void scheduler_set_first_arg(void *first_arg, struct registers *regs);

/*
 * adds a task to the sleep queue that will sleep for time scheduler ticks
 */
void add_sleep_queue(struct task *task, size_t time);

struct task *new_task_args(ptr ip, void *args);

void ktask_run(void (*task)(void *), void *args);

struct task_queue;
void task_queue_add_current(struct task_queue *queue);

#endif
