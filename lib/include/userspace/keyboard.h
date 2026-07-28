#ifndef USERSPACE_KEYBOARD_H
#define USERSPACE_KEYBOARD_H

#include <stdbool.h>
#include <types.h>

struct keyboard
{
    bool shift_pressed;
    bool control_pressed;
    int keyboard_fd;
};

struct keyboard *userspace_keyboard_alloc(const char *pathname);
char userspace_keyboard_process_scancode(struct keyboard *keyboard,
                                         u8 scancode);

#define USERSPACE_KEYBOARD_SHIFT_ON 1
#define USERSPACE_KEYBOARD_SHIFT_OFF 2

#define USERSPACE_KEYBOARD_CONTROL_ON 3
#define USERSPACE_KEYBOARD_CONTROL_OFF 4

#endif
