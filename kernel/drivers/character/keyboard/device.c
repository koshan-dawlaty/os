#include "vfs.h"
#include "wait_queue.h"
#include <cdev.h>
#include <drivers/character/terminal.h>
#include <errno.h>
#include <stdio.h>
#include <types.h>

int keyboard_read(struct cdev *dev, u8 *buf, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        buf[i] = getchar();
    }

    return len;
}

int keyboard_write(struct cdev *dev, u8 *buf, size_t len)
{
    /* cannot write to the keyboard */
    return -EINVAL;
}

/* TODO: move the full keyboard driver out of x86 and into here so these can be
 * in this file */
extern struct wait_queue keyboard_queue;
extern u8 keyboard_buffer_head;
extern u8 keyboard_buffer_tail;

poll_flags keyboard_poll(struct cdev *dev, struct wait_queue **queue)
{
    /* TODO: this should be a queue in the device */
    *queue = &keyboard_queue;
    return keyboard_buffer_head != keyboard_buffer_tail;
}

struct device *keyboard_device = NULL;

struct cdev_functions keyboard_cdev_functions = {
    .read = keyboard_read, .write = keyboard_write, .poll = keyboard_poll};

struct device *get_keyboard()
{
    if (keyboard_device == NULL)
    {
        keyboard_device = new_cdev(NULL, keyboard_cdev_functions);
    }

    return keyboard_device;
}
