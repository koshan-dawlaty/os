#include "wm_lib.h"
#include "fcntl.h"
#include "lib/framebuffer.h"
#include <err_ptr.h>
#include <errno.h>
#include <lib/log.h>
#include <sched.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
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
                /* client_draw_pixel(client, x + 4, y, 0); */
            }
        }
    }
}

void client_draw_string(struct wm_client client, struct PSF_font *font,
                        const char *str, size_t x, size_t y)
{
    for (size_t i = 0; i < strlen(str); i++)
    {
        client_draw_char(client, font, str[i], x + (i * font->header.width), y);
    }
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
    if (retval == -ENOTSOCK)
    {
        return 0xFF;
    }

    return out;
}

static struct wm_client wm_client_connect_impl(size_t height, size_t width,
                                               bool is_nonblock,
                                               bool auto_focus)
{
    int server = open("/wm_socket", O_RDONLY);
    int server_sock = connect(server);

    int key_server = open("/input_socket", O_RDONLY);
    int key_socket = connect(key_server);

    struct wm_inital win = {.height = height,
                            .width = width,
                            .is_nonblock = is_nonblock,
                            .auto_foucs = auto_focus};
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
        .mem_id = second_message.mem_id,
    };

    return out;
}

struct wm_client wm_client_connect_nonblock(size_t height, size_t width)
{
    return wm_client_connect_impl(height, width, true, false);
}

struct wm_client wm_client_connect(size_t height, size_t width)
{
    return wm_client_connect_impl(height, width, false, false);
}

struct wm_client wm_client_connect_focus(size_t height, size_t width)
{
    return wm_client_connect_impl(height, width, false, true);
}

void wm_get_message(struct wm_client *client, struct wm_message *message)
{
    struct wm_message_header header;
    recv(client->key_socket, &header, sizeof(struct wm_message_header));
    message->message_data = malloc(header.len);
    recv(client->key_socket, message->message_data, header.len);

    message->header = header;
    if (message->header.type == WM_MESSAGE_RESIZE)
    {

        struct wm_message_resize *resize = message->message_data;
        void *new_buf;

        shared_mem_get(client->mem_id, &new_buf);
        framebuffer_resize(&client->fb, resize->new_w, resize->new_h, new_buf);
        client->fb.buffer = new_buf;
        client->fb.width = resize->new_w;
        client->fb.height = resize->new_h;
        client->fb.pitch = resize->new_w * 4;

        int ack_code = 123;
        send(client->key_socket, &ack_code, sizeof(int));

        /* wm_render(*client); */
    }
    /* logd("got data:%s, %d", message->message_data, header.len); */
}
