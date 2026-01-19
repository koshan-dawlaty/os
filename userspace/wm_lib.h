#ifndef WM_LIB_H
#define WM_LIB_H

#include <stddef.h>
#include <stdint.h>

#define __packed __attribute__((packed))

struct window
{
    size_t x;
    size_t y;
    size_t w;
    size_t h;

    void *buffer;
};

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

struct wm_inital
{
    size_t height;
    size_t width;
} __packed;

struct wm_second
{
    int mem_id;
};

struct wm_client_message
{
    int code;
} __packed;

void framebuffer_pixel_write(struct framebuffer *f, uint8_t *pixel, int color);
void framebuffer_draw_rect(struct framebuffer *f, size_t x, size_t y,
                           size_t width, size_t height, int color);

#endif
