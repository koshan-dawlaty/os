#include "container_of.h"
#include "lib/log.h"
#include "vfs.h"
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

/* TODO: this should be defined in the device, so multiple mice can exist */
WAIT_QUEUE_DEFINE(mouse_waitqueue);

#define PS2_MOUSE_LEFT_CLICK (1 << 0)
#define PS2_MOUSE_RIGHT_CLICK (1 << 1)
#define PS2_MOUSE_MIDDLE_CLICK (1 << 2)
#define PS2_MOUSE_ALWAYS_ONE (1 << 3)
#define PS2_MOUSE_X_SIGN (1 << 4)
#define PS2_MOUSE_X_OVERFLOW (1 << 6)
#define PS2_MOUSE_Y_OVERFLOW (1 << 7)

struct mouse_event ps2_mouse_event = {0, 0, {false, false, false}, 0};
struct mouse_event ps2_mouse_event_prev = {0, 0, {false, false, false}, 0};

static bool ps2_mouse_has_changed()
{
    return !(ps2_mouse_event.x_change == ps2_mouse_event_prev.x_change &&
             ps2_mouse_event.y_change == ps2_mouse_event_prev.y_change &&
             ps2_mouse_event.buttons.left_click ==
                 ps2_mouse_event_prev.buttons.left_click &&
             ps2_mouse_event.buttons.right_click ==
                 ps2_mouse_event_prev.buttons.right_click &&
             ps2_mouse_event.buttons.middle_click ==
                 ps2_mouse_event_prev.buttons.middle_click &&
             ps2_mouse_event.scroll == ps2_mouse_event_prev.scroll);
}

void mouse_ps2_interrupt()
{
    /* printf("mouse\n"); */

    u8 buttons = inb(0x60);

    if (!(buttons & (1 << 3)))
    {
        return;
    }
    int8_t x = inb(0x60);
    int8_t y = inb(0x60);
    int8_t scroll = inb(0x60);
    /* logd("%x %x %x %x", buttons, x, y, scroll); */

    if (!(buttons & PS2_MOUSE_ALWAYS_ONE))
    {
        return;
    }

    /* TODO: maybe use the overflow bit
     * i think it will make the mouse get offset from the real mouse less
     */
    if ((buttons & PS2_MOUSE_X_OVERFLOW) || (buttons & PS2_MOUSE_Y_OVERFLOW))
    {
        return;
    }

    ps2_mouse_event.buttons.left_click = (buttons & PS2_MOUSE_LEFT_CLICK) != 0;
    /* logd("left:%d", ps2_mouse_event.buttons.left_click); */
    ps2_mouse_event.buttons.right_click =
        (buttons & PS2_MOUSE_RIGHT_CLICK) != 0;
    ps2_mouse_event.buttons.middle_click =
        (buttons & PS2_MOUSE_MIDDLE_CLICK) != 0;

    /* logd("left click in mouse interrupt handler:%d", */
    /*      ps2_mouse_event.buttons.left_click); */

    ps2_mouse_event.x_change += x;
    ps2_mouse_event.y_change -= y;

    if (scroll & 1)
    {
        if ((scroll & 0b1111) == 1)
        {
            ps2_mouse_event.scroll = -1;
        }
        else if ((scroll & 0b1111) == 0xF)
        {
            ps2_mouse_event.scroll = 1;
        }
        else
        {
            /* ps2_mouse_event.scroll = 0; */
        }
    }
    else
    {
        /* ps2_mouse_event.scroll = 0; */
    }

    if (!WAIT_QUEUE_EMPTY(mouse_waitqueue) && ps2_mouse_has_changed())
    {
        wait_queue_done(&mouse_waitqueue);
    }
}

int mouse_read(struct cdev *dev, u8 *buf, size_t len)
{

    if (!(ps2_mouse_has_changed()))
    {
        wait_queue_wait(&mouse_waitqueue);
    }

    size_t bytes_written = MIN(len, sizeof(struct mouse_event));
    memcpy(buf, &ps2_mouse_event, bytes_written);

    ps2_mouse_event.x_change = 0;
    ps2_mouse_event.y_change = 0;

    ps2_mouse_event.buttons = (struct mouse_buttons){false, false, false};
    ps2_mouse_event.scroll = 0;

    ps2_mouse_event_prev = ps2_mouse_event;

    return bytes_written;
}

int mouse_write(struct cdev *dev, u8 *buf, size_t len)
{
    return -EINVAL;
}

poll_flags mouse_poll(struct cdev *dev, struct wait_queue **queue)
{
    *queue = &mouse_waitqueue;

    poll_flags out = 0;
    if (ps2_mouse_has_changed())
    {
        out |= POLL_READ;
    }
    return out;
}

struct cdev_functions mouse_cdev_functions = {
    .read = mouse_read, .write = mouse_write, .poll = mouse_poll};

struct device *mouse_device = NULL;
struct device *mouse_get()
{
    if (mouse_device == NULL)
    {
        /* FIXME: all of these ps2_read_poll() calls should be checked to return
         * 0xFA, there should probably be a better way of sending a command to a
         * generic ps/2 device */
        ps2_write_dev(0xF3, true);
        ps2_read_poll(true);
        ps2_write_dev(0xC8, true);
        ps2_read_poll(true);

        ps2_write_dev(0xF3, true);
        ps2_read_poll(true);
        ps2_write_dev(0x64, true);
        ps2_read_poll(true);

        ps2_write_dev(0xF3, true);
        ps2_read_poll(true);
        ps2_write_dev(0x50, true);
        ps2_read_poll(true);

        ps2_write_dev(0xF2, true);
        ps2_read_poll(true);
        if (ps2_read_poll(true) != 3)
        {
            PANIC("ps/2 mouse init failed");
        }

        mouse_device = new_cdev(NULL, mouse_cdev_functions);
    }

    return mouse_device;
}
