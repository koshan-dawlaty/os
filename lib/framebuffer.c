#include "lib/utils.h"
#include <compiler.h>
#include <lib/framebuffer.h>
#include <lib/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <types.h>

void framebuffer_pixel_write(struct framebuffer *f, u8 *pixel, int color)
{
    pixel[0] = color & 255;         // BLUE
    pixel[1] = (color >> 8) & 255;  // GREEN
    pixel[2] = (color >> 16) & 255; // RED
}

void framebuffer_draw_rect(struct framebuffer *f, size_t x, size_t y,
                           size_t width, size_t height, int color)
{

    if (unlikely(x > f->width || y > f->height))
    {
        return;
    }

    width = MIN(x + width, f->width);
    height = MIN(y + height, f->height);

    size_t location = y * f->pitch + x * f->pixel_width;

    for (size_t y_pos = y; y_pos < height + y + 1; y_pos++)
    {
        for (size_t x_pos = x; x_pos < width + x; x_pos++)
        {
            u8 *pixel = f->buffer + location;
            framebuffer_pixel_write(f, pixel, color);
            location += f->pixel_width;
        }
        location = y_pos * f->pitch + x * f->pixel_width;
    }
}

u32 framebuffer_get_pixel(struct framebuffer *fb, size_t x, size_t y)
{
    if (unlikely(x > fb->width || y > fb->height || x < 0 || y < 0))
    {
        /* for right now just ignore invalid positions */
        return 0;
    }

    size_t location = y * fb->pitch + x * fb->pixel_width;

    u8 *pixel = fb->buffer + location;

    return *(u32 *)(pixel);
}

void framebuffer_draw_line(struct framebuffer *f, int x1, int y1, int x2,
                           int y2, int color)
{

    int x_dist = (x1 - x2);
    int y_dist = (y1 - y2);

    if (x_dist == 0)
    {

        size_t y_min = MIN(y1, y2);
        for (size_t i = 0; i < abs(y_dist); i++)
        {
            framebuffer_put_pixel(f, color, x1, y_min + i);
        }
        return;
    }

    double y_increment = (double)y_dist / (double)x_dist;
    /* logd("y_increment:%f", y_increment); */

    double x_min = MIN(x1, x2);
    double y_min = MIN(y1, y2);

    for (double x = x_min; x < MAX(x1, x2); x++)
    {
        /* framebuffer_put_pixel(f, color, x, y_increment * (x - x_min) +
         * y_min); */
        framebuffer_put_pixel(f, color, x, y_increment * (x - x1) + y1);
    }
}
