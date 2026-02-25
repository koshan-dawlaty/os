#include "wm_lib.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

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

void client_draw_char(struct client_window *w, struct PSF_font *font, char c,
                      size_t x, size_t y)
{
    struct PSF_header header = font->header;
    uint8_t *char_start = (header.bytes_per_glyph * c) + font->content;

    for (size_t y_pos = y; y_pos < y + header.height; y_pos++)
    {
        for (size_t x_pos = x; x_pos < x + header.width; x_pos++)
        {
            if (char_start[y_pos - y] & (1 << (7 - (x_pos - x))))
            {
                client_draw_pixel(w, x_pos + 4, y_pos, 0xFFFFFF);
            }
            else
            {
                /* client_draw_pixel(w, x + 4, y, 0); */
            }
        }
    }
}

#define PSF_FONT_MAGIC 0x864ab572
struct PSF_font *font_parse(int fd)
{
    struct PSF_font *out = malloc(sizeof(struct PSF_font));

    pread(fd, &out->header, sizeof(struct PSF_header), 0);

    if (out->header.magic != PSF_FONT_MAGIC)
    {
        return NULL;
    }

    size_t content_len = out->header.bytes_per_glyph * out->header.num_glyph;
    out->content = malloc(content_len);
    pread(fd, out->content, content_len, out->header.header_size);

    return out;
}

void wm_render(struct wm_client client)
{
    int render_code = WM_RES_RENDER;
    send(client.socket, &render_code, sizeof(int));

    int ack_code;
    recv(client.socket, &ack_code, sizeof(int));
    if (ack_code != 3)
    {
        printf("server responded with invalid acknowledgment code:%d\n",
               ack_code);
        exit(1);
    }
}

char wm_get_key(struct wm_client client)
{
    int key_code = WM_RES_KEY;
    send(client.socket, &key_code, sizeof(int));

    char out;
    recv(client.socket, &out, sizeof(char));

    return out;
}

struct wm_client wm_client_connect(size_t height, size_t width)
{
    /* TODO: assumes socket is 0, make it something in the fs */
    int server_sock;
    recv(0, &server_sock, sizeof(int));

    struct wm_inital win = {.height = height, .width = width};
    send(server_sock, &win, sizeof(struct wm_inital));

    /* TODO: make this a recv() acknowledge like for rendering */
    sched_yield();

    struct wm_second second_message;
    recv(server_sock, &second_message, sizeof(int));

    void *framebuffer;
    shared_mem_get(second_message.mem_id, &framebuffer);

    struct client_window w = {
        .w = win.width, .h = win.height, .buffer = framebuffer};

    struct wm_client out = {.win = w, .socket = server_sock};

    return out;
}
