#include "types.h"
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <userspace/keyboard.h>

static unsigned char keyboard_map[128] = {
    0,   27,   '1',  '2', '3',  '4', '5', '6', '7', '8', '9', '0', '-',
    '=', '\b', '\t', 'q', 'w',  'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',
    '[', ']',  '\n', 0,   'a',  's', 'd', 'f', 'g', 'h', 'j', 'k', 'l',
    ';', '\'', '`',  0,   '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',',
    '.', '/',  0,    '*', 0,    ' ', 0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,    0,   0,   0,   0,   '-', 0,   0,   0,
    '+', 0,    0,    0,   0,    0,   0,   0,   0,   0,   0,   0, /* All other
                                                                    keys are
                                                                    undefined */
};

static unsigned char keyboard_map_shift[128] = {
    0,   27,   '!',  '@', '#', '$', '%', '^', '&', '*', '(', ')', '_',
    '+', '\b', '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P',
    '{', '}',  '\n', 0,   'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L',
    ':', '\"', '~',  0,   '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<',
    '>', '?',  0,    '*', 0,   ' ', 0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,   0,   0,   0,   0,   '-', 0,   0,   0,
    '+', 0,    0,    0,   0,   0,   0,   0,   0,   0,   0,   0, /* All other
                                                                   keys are
                                                                   undefined */
};

struct keyboard *userspace_keyboard_alloc(const char *pathname)
{
    struct keyboard *keyboard = malloc(sizeof(struct keyboard));
    keyboard->keyboard_fd = open(pathname, O_RDONLY);
    keyboard->shift_pressed = false;

    return keyboard;
}

char userspace_keyboard_process_scancode(struct keyboard *keyboard, u8 scancode)
{

    /* caps lock to escape */
    if (scancode == 0x3A)
    {
        scancode = 1;
    }
    else if (scancode == 0x81)
    {
        scancode = 0xBA;
    }

    if (scancode == 0x2A)
    {
        keyboard->shift_pressed = true;
        return USERSPACE_KEYBOARD_SHIFT_ON;
    }
    else if (scancode == 0xAA)
    {
        keyboard->shift_pressed = false;
        return USERSPACE_KEYBOARD_SHIFT_OFF;
    }
    else if (scancode == 0x1D)
    {
        keyboard->control_pressed = true;
        return USERSPACE_KEYBOARD_CONTROL_ON;
    }
    else if (scancode == 0x9D)
    {
        keyboard->control_pressed = false;
        return USERSPACE_KEYBOARD_CONTROL_OFF;
    }

    if (scancode < 0x81)
    {

        char c;
        if (keyboard->shift_pressed)
        {
            c = keyboard_map_shift[scancode];
        }
        else
        {
            c = keyboard_map[scancode];
        }

        return c;
    }

    return 0;
}
