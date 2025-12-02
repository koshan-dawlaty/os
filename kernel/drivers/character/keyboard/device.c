#include <cdev.h>
#include <drivers/character/terminal.h>
#include <errno.h>
#include <kernel/types.h>
#include <stdio.h>

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

struct device *keyboard_device = NULL;

struct cdev_functions keyboard_cdev_functions = {
    .read = keyboard_read,
    .write = keyboard_write,
};

struct device *get_keyboard()
{
    if (keyboard_device == NULL)
    {
        keyboard_device = new_cdev(NULL, keyboard_cdev_functions);
    }

    return keyboard_device;
}
