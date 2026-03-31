#include "wm_lib.h"
#include "fcntl.h"
#include <err_ptr.h>
#include <errno.h>
#include <lib/log.h>
#include <sched.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

void client_draw_char(struct wm_client client, struct PSF_font *font, char c,
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
                client_draw_pixel(client, x_pos, y_pos, 0xFFFFFF);
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

    if (fd < 0)
    {
        return ERR_PTR(-EBADF);
    }

    size_t read_size = pread(fd, &out->header, sizeof(struct PSF_header), 0);

    if (read_size != sizeof(struct PSF_header))
    {
        return ERR_PTR(-EWRONGSIZE);
    }

    if (out->header.magic != PSF_FONT_MAGIC)
    {
        return ERR_PTR(-EINVAL);
    }

    size_t content_len = out->header.bytes_per_glyph * out->header.num_glyph;
    out->content = malloc(content_len);
    pread(fd, out->content, content_len, out->header.header_size);

    return out;
}

void wm_render(struct wm_client client)
{
    int render_code = WM_RES_RENDER;
    int retval = send(client.socket, &render_code, sizeof(int));
    if (retval == -ENOTSOCK)
    {
        logd("AAAA");
    }

    int ack_code;
    retval = recv(client.socket, &ack_code, sizeof(int));

    if (retval == -ENOTSOCK)
    {
        logd("AAAA");
    }
    if (ack_code != 3)
    {
        printf("server responded with invalid acknowledgment code:%d\n",
               ack_code);
        exit(1);
    }
}

char wm_get_key(struct wm_client client)
{
    /* int key_code = WM_RES_KEY; */
    /* int retval = send(client.socket, &key_code, sizeof(int)); */
    /* if (retval == -ENOTSOCK) */
    /* { */
    /*     return 0xFF; */
    /* } */

    /* char out; */
    /* retval = recv(client.socket, &out, sizeof(char)); */
    /* if (retval == -ENOTSOCK) */
    /* { */
    /*     return 0xFF; */
    /* } */
    char out;
    int retval = recv(client.key_socket, &out, sizeof(char));
    if (retval == -ENOTSOCK)
    {
        return 0xFF;
    }

    return out;
}
char wm_get_key_nonblock(struct wm_client client)
{
    int key_code = WM_RES_KEY_NONBLOCK;
    int retval = send(client.socket, &key_code, sizeof(int));
    if (retval == -ENOTSOCK)
    {
        return 0xFF;
    }

    char out;
    retval = recv(client.socket, &out, sizeof(char));
    logd("%d", retval);
    if (retval == -ENOTSOCK)
    {
        return 0xFF;
    }

    return out;
}

static struct wm_client wm_client_connect_impl(size_t height, size_t width,
                                               bool is_nonblock)
{
    int server = open("/wm_socket", O_RDONLY);
    int server_sock = connect(server);

    int key_server = open("/input_socket", O_RDONLY);
    int key_socket = connect(key_server);

    struct wm_inital win = {
        .height = height, .width = width, .is_nonblock = is_nonblock};
    send(server_sock, &win, sizeof(struct wm_inital));

    /* TODO: make this a recv() acknowledge like for rendering */
    sched_yield();

    struct wm_second second_message;
    recv(server_sock, &second_message, sizeof(int));

    void *framebuffer;
    shared_mem_get(second_message.mem_id, &framebuffer);

    size_t fb_width = win.width;
    struct wm_client out = {
        .fb = FRAMEBUFFER_INIT(fb_width, win.height, fb_width * 4, 32, 4,
                               framebuffer),
        .socket = server_sock,
        .key_socket = key_socket,
    };

    return out;
}

struct wm_client wm_client_connect_nonblock(size_t height, size_t width)
{
    return wm_client_connect_impl(height, width, true);
}

struct wm_client wm_client_connect(size_t height, size_t width)
{
    return wm_client_connect_impl(height, width, false);
}
