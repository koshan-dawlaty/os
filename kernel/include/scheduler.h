#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "lib/log.h"
#include <compiler.h>
#include <kernel/addr_space.h>
#include <kernel/paging.h>
#include <kernel/types.h>
#include <lib/list.h>
#include <stdbool.h>
#include <stdint.h>
#include <task_queue.h>
#include <utils.h>
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
    struct list2c list;

    struct addr_space addr_space;

    void *stack;
    struct registers regs;
    ptr bp;
    struct dentry *working_dir;
    struct fdlist *descriptors;

    int preempt_count; /* using int makes it easier to see if it is negative */

    void *start_addr; /* used for debugging */

    /*
     * if true, the task can be safely removed from / added to the scheduler
     * queue, the task might already be on the scheduler queue
     *
     * used by the poll implementation with waiting on multiple wait queues
     */
    bool can_sleep;
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
void schedule_impl();

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
/* TODO: make this a good api */
void remove_task_add_queue(struct task *task, struct task_queue *queue);

static inline void preempt_disable()
{
    struct task *task = get_current_task();
    logd("%p", task);
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
static inline void preempt_enable()
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

/*
 * removes task from scheduler queue, and frees it atomically
 */
void task_remove_free(struct task *task);

struct sched_queue
{
    struct task_queue_entry *current_task;
    struct task_queue queue;
};
#define SCHED_QUEUE_CURRENT_TASK(q) (q.current_task->task)

#endif
