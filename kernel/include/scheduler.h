#ifndef SCHEDULER_H
#define SCHEDULER_H
#include "kernel/paging.h"
#include <stdint.h>

struct task
{
    uint32_t instruction_pointer;
    struct task *next;

    struct pagedir *pgdir;
};

/*
 * ip is the current instruction pointer
 */
void switch_tasks(uint32_t ip);
void add_task(struct task *task);

/*
 * initializes the page directory as a free page
 */
struct task *new_task(uint32_t ip);

#endif
