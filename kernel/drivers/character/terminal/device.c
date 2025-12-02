#include <cdev.h>
#include <errno.h>
#include <kernel/types.h>
#include <stdio.h>

int terminal_read(struct cdev *dev, u8 *buf, size_t len)
{
    /* cant write to terminal */
    return -EINVAL;
}

/* TODO: there is another function called this */
int terminal_write2(struct cdev *dev, u8 *buf, size_t len)
{

    for (size_t i = 0; i < len; i++)
    {
        putchar(buf[i]);
    }

    return len;
}

struct device *terminal_device = NULL;

struct cdev_functions terminal_cdev_functions = {
    .read = terminal_read,
    .write = terminal_write2,
};

struct device *get_terminal()
{
    if (terminal_device == NULL)
    {
        terminal_device = new_cdev(NULL, terminal_cdev_functions);
    }

    return terminal_device;
}
