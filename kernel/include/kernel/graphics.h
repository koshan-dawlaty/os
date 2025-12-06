#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <kernel/types.h>
#include <stddef.h>

struct framebuffer
{
    size_t width;
    size_t height;
    size_t pitch;
    size_t depth;
    size_t pixel_width; /* bytes */
    void *buffer;
};

void framebuffer_put_pixel(struct framebuffer *f, int color, int x, int y);
void framebuffer_pixel_write(struct framebuffer *f, u8 *pixel, int color);

void framebuffer_draw_rect(struct framebuffer *f, size_t x, size_t y,
                           size_t width, size_t height, int color);

void framebuffer_draw_char(struct framebuffer *f, char c, size_t x, size_t y);

void graphics_init();
#endif
