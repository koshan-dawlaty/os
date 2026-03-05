#ifndef LIB_FRAMEBUFFER_H
#define LIB_FRAMEBUFFER_H

#include <compiler.h>
#include <stddef.h>
#include <types.h>

struct framebuffer
{
    size_t width;
    size_t height;
    size_t pitch;
    size_t depth;
    size_t pixel_width; /* bytes */
    void *buffer;
} __packed;

void framebuffer_put_pixel(struct framebuffer *f, int color, int x, int y);
void framebuffer_pixel_write(struct framebuffer *f, u8 *pixel, int color);
void framebuffer_draw_rect(struct framebuffer *f, size_t x, size_t y,
                           size_t width, size_t height, int color);

#endif
