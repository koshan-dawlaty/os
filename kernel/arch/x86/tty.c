#include "kernel/tty.h"
#include "ansi.h"
#include "kernel/graphics.h"
#include "vga.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <threads.h>
#define TERMINAL_EMPTY_CHAR ' '

struct framebuffer *tty_framebuffer;

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;

static size_t terminal_row;
static size_t terminal_column;
static uint8_t terminal_color;
static uint16_t *terminal_buffer;

void move_line(int dest_column, int source_column)
{

    if (tty_framebuffer == NULL)
    {
        uint16_t *source = (terminal_buffer + source_column * VGA_WIDTH);
        uint16_t *dest = (terminal_buffer + dest_column * VGA_WIDTH);
        memcpy(dest, source, VGA_WIDTH);
    }
    else
    {
        u8 *source =
            tty_framebuffer->buffer + source_column * tty_framebuffer->pitch;
        u8 *dest =
            tty_framebuffer->buffer + dest_column * tty_framebuffer->pitch;

        memcpy(dest, source, tty_framebuffer->pitch);
    }
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

void terminal_putcharat(char c, size_t x, size_t y)
{
    if (tty_framebuffer != NULL)
    {
        framebuffer_draw_char(tty_framebuffer, c, x * 8, y * 16);
        /* framebuffer_put_pixel(tty_framebuffer, 0xFFFFFF, x, y); */
    }
    else
    {
        /* terminal_buffer[terminal_column * VGA_WIDTH + terminal_row] = */
        /*     vga_entry(c, terminal_color); */
    }
}

bool parsing_ansi = false;

void terminal_putchar(char c)
{

    if (parsing_ansi)
    {
        parsing_ansi = !parse_ansi(c);
        return;
    }
    if (c == '\033')
    {
        parsing_ansi = true;
        return;
    }

    if (c == '\n')
    {
        terminal_row = 0;
        terminal_column++;

        // TODO: this causes triple faults
        return;
    }

    // backspace
    if (c == '\b')
    {

        /* terminal_buffer[terminal_column * VGA_WIDTH + terminal_row] = */
        /*     vga_entry(TERMINAL_EMPTY_CHAR, terminal_color); */
        /* terminal_putcharat(TERMINAL_EMPTY_CHAR, terminal_row,
         * terminal_column); */
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

    terminal_putcharat(c, terminal_row, terminal_column);
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
    terminal_column--;
}

void terminal_clear()
{
    terminal_row = 0;
    terminal_column = 0;

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
