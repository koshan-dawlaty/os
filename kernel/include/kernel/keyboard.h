#ifndef KEYBOARD_H
#define KEYBOARD_H

#define KEYBOARD_BUFFER_SIZE 256

static char keyboard_buffer[KEYBOARD_BUFFER_SIZE];
extern volatile int key_available;

char pop_keyboard_buffer();

void keyboard_set_default();
void keyboard_reset();

#endif
