#include <cdev.h>
#include <device.h>
#include <drivers/character/mouse.h>
#include <errno.h>
#include <interrupts/idt.h>
#include <interrupts/pic.h>
#include <kernel/interrupts.h>
#include <kernel/ps2.h>
#include <lib/list.h>
#include <lib/utils.h>
#include <panic.h>
#include <scheduler.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
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
#define PS2_MOUSE_Y_OVERFLOW (1 << 7)

struct mouse_event ps2_mouse_event = {0, 0, {false, false, false}};

INT_HANDLER(mouse_interrupt_handler, {
    u8 buttons = inb(0x60);
    int8_t x = inb(0x60);
    int8_t y = inb(0x60);

    /* TODO: maybe use the overflow bit
     * i think it will make the mouse get offset from the real mouse less
     */
    if (!(buttons & PS2_MOUSE_ALWAYS_ONE) || (buttons & PS2_MOUSE_X_OVERFLOW) ||
        (buttons & PS2_MOUSE_Y_OVERFLOW))
    {
        goto end;
    }

    if (buttons & PS2_MOUSE_LEFT_CLICK)
    {
        ps2_mouse_event.buttons.left_click = true;
    }
    if (buttons & PS2_MOUSE_RIGHT_CLICK)
    {
        ps2_mouse_event.buttons.right_click = true;
    }

    if (buttons & PS2_MOUSE_MIDDLE_CLICK)
    {
        ps2_mouse_event.buttons.middle_click = true;
    }

    ps2_mouse_event.x_change += x;
    ps2_mouse_event.y_change -= y;

    if (!WAIT_QUEUE_EMPTY(mouse_waitqueue))
    {
        wait_queue_done(&mouse_waitqueue);
    }

end:
    PIC_sendEOI(12);
});

int mouse_read(struct cdev *dev, u8 *buf, size_t len)
{

    if (MOUSE_EVENT_EMPTY(ps2_mouse_event))
    {
        wait_queue_wait(&mouse_waitqueue);
    }

    size_t bytes_written = MIN(len, sizeof(struct mouse_event));
    memcpy(buf, &ps2_mouse_event, bytes_written);

    ps2_mouse_event.x_change = 0;
    ps2_mouse_event.y_change = 0;

    ps2_mouse_event.buttons = (struct mouse_buttons){false, false, false};

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
