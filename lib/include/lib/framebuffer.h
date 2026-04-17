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

#define FRAMEBUFFER_INIT(w, h, _pitch, _depth, _pixel_width, _buffer)          \
    ((struct framebuffer){.width = w,                                          \
                          .height = h,                                         \
                          .pitch = _pitch,                                     \
                          .depth = _depth,                                     \
                          .pixel_width = _pixel_width,                         \
                          .buffer = _buffer})

void framebuffer_pixel_write(struct framebuffer *f, u8 *pixel, int color);
void framebuffer_draw_rect(struct framebuffer *f, size_t x, size_t y,
                           size_t width, size_t height, int color);

static inline void framebuffer_put_pixel(struct framebuffer *f, int color,
                                         int x, int y)
{
    if (unlikely(x > f->width || y > f->height || x < 0 || y < 0))
    {
        /* for right now just ignore invalid positions */
        return;
    }

    size_t location = y * f->pitch + x * f->pixel_width;

    u8 *pixel = f->buffer + location;

    framebuffer_pixel_write(f, pixel, color);
}

/*
 * gets a pixel from a framebuffer
 * note that this might be VERY slow if the framebuffer is video memory
 */
u32 framebuffer_get_pixel(struct framebuffer *fb, size_t x, size_t y);

#endif
