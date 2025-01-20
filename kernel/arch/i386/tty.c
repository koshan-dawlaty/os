#include "kernel/tty.h"
#include "vga.h"
#include <stdio.h>
#include <string.h>
#ifndef _KERNEL_TTY_H
#define _KERNEL_TTY_H

#include <stddef.h>
#include <stdint.h>

void terminal_initialize(void);
void terminal_putchar(char c);
void terminal_write(const char *data, size_t size);
void terminal_writestring(const char *data);

#endif

#define TERMINAL_EMPTY_CHAR '_'

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;

static size_t terminal_row;
static size_t terminal_column;
static uint8_t terminal_color;
static uint16_t *terminal_buffer;

void move_line(int dest_column, int source_column)
{
    uint16_t *source = (terminal_buffer + source_column * VGA_WIDTH);
    uint16_t *dest = (terminal_buffer + dest_column * VGA_WIDTH);

    memcpy(dest, source, VGA_WIDTH);
}

void terminal_initialize()
{
    terminal_row = 0;
    terminal_column = 0;
    terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    terminal_buffer = VGA_MEMORY;

    for (size_t y = 0; y < VGA_HEIGHT; y++)
    {

        for (size_t x = 0; x < VGA_WIDTH; x++)
        {
            const size_t index = y * VGA_WIDTH + x;
            terminal_buffer[index] =
                vga_entry(TERMINAL_EMPTY_CHAR, terminal_color);
        }
    }
}

void terminal_putchar(char c)
{

    if (c == '\n')
    {
        terminal_row = 0;
        terminal_column++;
        // this causes triple faults
        return;
    }

    // backspace
    if (c == '\b')
    {

        terminal_buffer[terminal_column * VGA_WIDTH + terminal_row] =
            vga_entry(TERMINAL_EMPTY_CHAR, terminal_color);
        if (terminal_row > 0)
        {
            terminal_row--;
        }
        else if (terminal_column > 0)
        {
            terminal_column--;
        }
        return;
    }

    if (terminal_row == VGA_WIDTH)
    {
        terminal_row = 0;
        terminal_column++;
    }
    if (terminal_column == VGA_HEIGHT)
    {
        terminal_scroll_up();
    }
    else
    {
        terminal_row++;
    }

    terminal_buffer[terminal_column * VGA_WIDTH + terminal_row] =
        vga_entry(c, terminal_color);
}

void terminal_write(const char *data, size_t size)
{

    for (size_t i = 0; i < size; i++)
    {
        terminal_putchar(data[i]);
    }
}

void terminal_writestring(const char *data)
{
    terminal_write(data, strlen((char *)data));
}

void terminal_scroll_up()
{
    /* for (size_t i = VGA_HEIGHT - 2; i > 2; i--) */
    /* { */
    /*     move_line(i, i - 1); */
    /* } */

    for (size_t i = 0; i < VGA_HEIGHT; i++)
    {
        move_line(i, i + 1);
    }
    terminal_row = 0;
}
