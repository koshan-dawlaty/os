#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>
#define KEYBOARD_BUFFER_SIZE 256

static char keyboard_buffer[KEYBOARD_BUFFER_SIZE];
extern volatile int key_available;

char pop_keyboard_buffer();

void keyboard_set_default();
void keyboard_reset();

void keyboard_set_typematic(uint8_t rate, uint8_t delay);

#define KEYBOARD_LED_SCROLL_LOCK 1 << 0
#define KEYBOARD_LED_NUMBER_LOCK 1 << 1
#define KEYBOARD_LED_CAPS_LOCK 2 << 1

void keyboard_set_led(uint8_t led);

#endif
