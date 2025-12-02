#include "interrupts/idt.h"
#include "keyboard_map.h"
#include "x86_keyboard.h"
#include <kernel/keyboard.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "interrupts/pic.h"
#include "panic.h"
#include "utils.h"

volatile int key_available;

uint8_t keyboard_buffer_head = 0;
uint8_t keyboard_buffer_tail = 0;

void push_keyboard_buffer(char c)
{
    /* printf("keyboard isr\n"); */
    uint8_t next_head = (keyboard_buffer_head + 1) % KEYBOARD_BUFFER_SIZE;

    if (next_head != keyboard_buffer_tail)
    {
        keyboard_buffer[keyboard_buffer_head] = c;
        keyboard_buffer_head = next_head;
    }
    else
    {
        panic("keyboard buffer overflow!\n");
    }
}

char pop_keyboard_buffer()
{
    if (keyboard_buffer_head == keyboard_buffer_tail)
    {
        return 0; // empty
    }
    char c = keyboard_buffer[keyboard_buffer_tail];
    keyboard_buffer_tail = (keyboard_buffer_tail + 1) % KEYBOARD_BUFFER_SIZE;
    return c;
}

void keyboard_isr_enable()
{
    IRQ_clear_mask(1);
}
void keyboard_isr_disable()
{
    IRQ_set_mask(1);
}

ISR(keyboard_interrupt_handler, {
    uint8_t scancode = inb(0x60);
    if (scancode < 0x81)
    {
        char c = keyboard_map[scancode];

        key_available = 1;
        push_keyboard_buffer(c);
    }

    PIC_sendEOI(1);
})

// this will not work with 0xEE (echo) as it sends 0xEE instead of acknowledged
void keyboard_send_command(uint8_t val)
{
    outb(KEYBOARD_PORT_DATA, val);

    /* printf("%x\n", inb(0x60)); */
    uint8_t result;
    do
    {
        result = inb(KEYBOARD_PORT_DATA);
        /* printf("%x\n", result); */
    } while (result != KEYBOARD_ACKNOWLEDGED);
}

void keyboard_reset()
{
    keyboard_isr_disable();

    keyboard_send_command(0xFF);

    uint8_t result = inb(KEYBOARD_PORT_DATA);
    switch (result)
    {
    case 0xAA:
        break;
    case 0xFC:
    case 0xFD:
        printf("keyboard self test failed\n");
        break;
    default:
        printf("keyboard self test sent invalid byte: %x\n", result);
    }
    keyboard_isr_enable();
}

void keyboard_set_default()
{
    keyboard_send_command(0xF6);
}

void keyboard_set_typematic(uint8_t rate, uint8_t delay)
{
    uint8_t data_byte = (rate & 0xFF) | ((delay >> 4) & 0xFF);
    keyboard_send_command(0xF3);
    outb(KEYBOARD_PORT_DATA, data_byte);
}

void keyboard_set_led(uint8_t led)
{
    keyboard_send_command(0xED);
    outb(KEYBOARD_PORT_DATA, led);
}

void keyboard_init()
{

    keyboard_reset();
    keyboard_set_default();
    keyboard_set_typematic(0, 0);
    keyboard_set_led(KEYBOARD_LED_CAPS_LOCK);

    /*
push_keyboard_buffer('a');
push_keyboard_buffer('b');
push_keyboard_buffer('c');
push_keyboard_buffer('d');
push_keyboard_buffer('e');
push_keyboard_buffer('f');
    */
}

bool is_key_available()
{
    return keyboard_buffer_head != keyboard_buffer_tail;
}
