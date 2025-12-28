#include <mutex.h>
#include <panic.h>
#include <sched.h>
#include <scheduler.h>
#include <utils.h>
#include <vfs.h>

extern void (*kernel_task1)(void *);
extern void (*kernel_task2)(void *);
extern void (*kernel_task3)(void *);

void mutex_lock(struct mutex *m)
{
    irq_disable();

    void *addr = get_current_task()->start_addr;
    if (addr == &kernel_task2)
    {
    }

    if (!m->locked)
    {
        printf("lock 1, %p\n", get_current_task());
        m->locked = true;
    }
    else
    {
        printf("lock 2, %p\n", get_current_task());
        struct task *current = get_current_task();
        remove_task(current);
        task_queue_push(&m->queue, current);

        sched_yield();
    }

    irq_enable();
}

void mutex_unlock(struct mutex *m)
{
    irq_disable();

    if (!m->locked)
    {
        panic("mutex unlock may have been called twice on the same mutex!");
    }

    if (m->queue.head == NULL)
    {
        printf("unlock 1, %p\n", get_current_task());
        m->locked = false;
    }
    else
    {
        printf("unlock 2, %p\n", get_current_task());
        struct task *temp = task_queue_pop(&m->queue);
        add_task(temp);
        schedule();
    }

    irq_enable();
}
