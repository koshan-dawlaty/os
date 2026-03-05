#include <lib/framebuffer.h>
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
    size_t location = y * f->pitch + x * f->pixel_width;

    for (size_t y_pos = y; y_pos < height + y; y_pos++)
    {
        for (size_t x_pos = x; x_pos < width + x; x_pos++)
        {
            uint8_t *pixel = f->buffer + location;
            framebuffer_pixel_write(f, pixel, color);
            location += f->pixel_width;
        }
        location = y_pos * f->pitch + x * f->pixel_width;
    }
}

void framebuffer_put_pixel(struct framebuffer *f, int color, int x, int y)
{
    size_t location = y * f->pitch + x * f->pixel_width;

    u8 *pixel = f->buffer + location;

    framebuffer_pixel_write(f, pixel, color);
}
