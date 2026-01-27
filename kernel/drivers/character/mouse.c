#include <cdev.h>
#include <device.h>
#include <drivers/character/mouse.h>
#include <errno.h>
#include <interrupts/idt.h>
#include <interrupts/pic.h>
#include <kernel/ps2.h>
#include <lib/list.h>
#include <lib/utils.h>
#include <panic.h>
#include <scheduler.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
#include <userspace/mouse.h>
#include <utils.h>

struct mouse_queue_ent
{
    struct mouse_event ev;
    struct list2 list;
};

struct list2 *mouse_queue = NULL;

ISR(mouse_interrupt_handler, {
    u8 buttons = inb(0x60);
    u8 x = inb(0x60);
    u8 y = inb(0x60);

    struct mouse_queue_ent *ent = malloc(sizeof(struct mouse_queue_ent));

    ent->ev.x_change = x;
    ent->ev.y_change = y;
    ent->list = LIST2_INIT();

    if (mouse_queue == NULL)
    {
        mouse_queue = &ent->list;
    }
    else
    {
        list2_add(&ent->list, mouse_queue);
    }

    printf("isr:%p\n", ent->ev.x_change);

    PIC_sendEOI(12);
});

int mouse_read(struct cdev *dev, u8 *buf, size_t len)
{

    if (mouse_queue == NULL)
    {
        /* TODO: this task should be put on a waitqueue */
        while (mouse_queue == NULL)
        {
            schedule();
        }
    }

    struct list2 *removed = list2_pop(&mouse_queue);

    struct mouse_queue_ent *ent =
        LIST_ENTRY(removed, struct mouse_queue_ent, list);

    size_t bytes_written = MAX(len, sizeof(struct mouse_event));
    memcpy(&ent->ev, buf, bytes_written);

    return bytes_written;
}

int mouse_write(struct cdev *dev, u8 *buf, size_t len)
{
    return -EINVAL;
}

struct cdev_functions mouse_cdev_functions = {
    .read = mouse_read,
    .write = mouse_write,
};

struct device *mouse_device = NULL;
struct device *mouse_get()
{
    if (mouse_device == NULL)
    {
        mouse_device = new_cdev(NULL, mouse_cdev_functions);
    }

    return mouse_device;
}
