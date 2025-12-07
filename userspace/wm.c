#include <init.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* this is copied from the kernel definition, it might change and break
 * everything and that would be bad */
struct framebuffer
{
    size_t width;
    size_t height;
    size_t pitch;
    size_t depth;
    size_t pixel_width; /* bytes */
    void *buffer;
} __packed;

typedef uintptr_t ptr;
extern int syscall_wrapper_asm(ptr ax, ptr bx, ptr cx, ptr dx, ptr si, ptr di);

struct framebuffer *framebuffer_alloc()
{
    struct framebuffer *out = NULL;
    syscall_wrapper_asm(18, (ptr)&out, 0, 0, 0, 0);

    return out;
}

void framebuffer_pixel_write(struct framebuffer *f, uint8_t *pixel, int color)
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

int main()
{
    init_libc();

    struct framebuffer *buffer = framebuffer_alloc();
    printf("test\n");
    printf("%p\n", buffer);

    printf("%p\n", buffer->buffer);
    /* printf("%p\n", buffer->pitch); */
    framebuffer_draw_rect(buffer, 100, 100, 100, 100, 0xff0000);

    exit(0);
}
