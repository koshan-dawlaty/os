#include "unistd.h"
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
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <userspace/mouse.h>
#include <utils.h>
#include <wait_queue.h>

struct wait_queue mouse_waitqueue = WAIT_QUEUE_INIT();

#define PS2_MOUSE_LEFT_CLICK (1 << 0)
#define PS2_MOUSE_RIGHT_CLICK (1 << 1)
#define PS2_MOUSE_MIDDLE_CLICK (1 << 2)
#define PS2_MOUSE_ALWAYS_ONE (1 << 3)
#define PS2_MOUSE_X_SIGN (1 << 4)
#define PS2_MOUSE_X_OVERFLOW (1 << 6)

int mouse_x = 0;
int mouse_y = 0;

ISR(mouse_interrupt_handler, {
    u8 buttons = inb(0x60);
    int8_t x = inb(0x60);
    int8_t y = inb(0x60);

    if (!(buttons & PS2_MOUSE_ALWAYS_ONE) || (buttons & PS2_MOUSE_X_OVERFLOW))
    {
        goto end;
    }

    mouse_x += x;
    mouse_y += -y;

    if (!WAIT_QUEUE_EMPTY(mouse_waitqueue))
    {
        wait_queue_done(&mouse_waitqueue);
    }

end:
    PIC_sendEOI(12);
});

int mouse_read(struct cdev *dev, u8 *buf, size_t len)
{

    if (mouse_x == 0 && mouse_y == 0)
    {
        wait_queue_wait(&mouse_waitqueue);
    }

    struct mouse_event ev = {.x_change = mouse_x, .y_change = mouse_y};
    mouse_x = 0;
    mouse_y = 0;
    size_t bytes_written = MIN(len, sizeof(struct mouse_event));
    memcpy(buf, &ev, bytes_written);

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
