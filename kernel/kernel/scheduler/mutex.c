#include "sched.h"
#include <mutex.h>
#include <scheduler.h>
#include <utils.h>

void mutex_lock(struct mutex *m)
{
    irq_disable();

    if (!m->locked)
    {
        m->locked = true;
    }
    else
    {
        struct task *current = get_current_task();
        remove_task(current);
        struct task *temp = m->queue;
        m->queue = current;
        m->queue->next = temp;
        sched_yield();
    }

    irq_enable();
}

void mutex_unlock(struct mutex *m)
{
    irq_disable();

    if (m->queue == NULL)
    {
        m->locked = false;
    }
    else
    {
        struct task *temp = m->queue;
        m->queue = temp->next;

        add_task(temp);
    }

    irq_enable();
}
