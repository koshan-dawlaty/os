#include "lib/assert.h"
#include "userspace/mouse.h"
#include "wm_lib.h"
#include <compiler.h>
#include <container_of.h>
#include <err_ptr.h>
#include <extension.h>
#include <fcntl.h>
#include <init.h>
#include <lib/array.h>
#include <lib/log.h>
#include <lib/term.h>
#include <lib/utils.h>
#include <sched.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <types.h>
#include <unistd.h>
#include <userspace/syscall_wrappers.h>

size_t mouse_x = 0;
size_t mouse_y = 0;
size_t x = 0, y = 0;
struct PSF_font *font;
struct wm_client client;
struct terminal term;
int pipe_write;
int mutex; /* TODO: maybe have a mutex in the wm_client variable? */

void key_thread()
{
    while (1)
    {
        /* logd("getting key"); */
        /* user_mutex_lock(mutex); */
        struct wm_message message;
        wm_get_message(&client, &message);
        switch (message.header.type)
        {
        case WM_MESSAGE_KEY:
        {
            char c = *(char *)message.message_data;
            /* user_mutex_unlock(mutex); */
            /* logd("done getting key"); */
            if ((u8)c == 0xFF)
            {
                break;
            }
            write(pipe_write, &c, 1);
            /* printf("%c ", c); */
            terminal_addchar(c, &term);
            user_mutex_lock(mutex);
            wm_render(client);
            user_mutex_unlock(mutex);
            break;
        }
        case WM_MESSAGE_MOUSE:
        {
            struct mouse_event *ev = message.message_data;
            mouse_x += ev->x_change;
            mouse_y += ev->y_change;
            /* logd("mouse at %d %d", mouse_x, mouse_y); */
            /* client_draw_string(client, font, "this is the mouse", mouse_x, */
            /*                    mouse_y); */
            /* client_draw_pixel(client, mouse_x, mouse_y, 0xFF00FF); */

            /* user_mutex_lock(mutex); */
            /* wm_render(client); */
            /* user_mutex_unlock(mutex); */
        }
        }
    }
    loge("bad key");
    exit(1);
}

u32 bg_color = 0x4b3973;
u32 cursor_color = 0x9f1ec9;

void wm_terminal_draw_char_at(char character, size_t x, size_t y,
                              bool is_cursor)
{
    /* TODO: client_draw_char, and the framebuffer draw_char function should be
     * able to draw a background with the character */
    if (is_cursor)
    {
        client_draw_rect(client, x * font->header.width,
                         y * font->header.height, font->header.width,
                         font->header.height, cursor_color);
    }
    else
    {
        client_draw_rect(client, x * font->header.width,
                         y * font->header.height, font->header.width,
                         font->header.height, bg_color);
    }
    client_draw_char(client, font, character, x * font->header.width,
                     y * font->header.height);
}

size_t wm_terminal_get_width()
{
    return client.fb.width / font->header.width;
}

size_t wm_terminal_get_height()
{
    return (client.fb.height / font->header.height) - 2;
}

const struct terminal_operations wm_term_operations = {
    wm_terminal_draw_char_at, wm_terminal_get_width, wm_terminal_get_height};

int main()
{

    init_libc();

    /* int serial = open("/serial0", O_RDWR); */

    /* write(serial, "abc", 3); */
    /* printf("done\n"); */

    /* char *buf = malloc(4); */
    /* read(serial, buf, 4); */
    /* printf("read:%s\n", buf); */

    mutex = user_mutex_create();

    /* clone((ptr)t1); */
    /* printf("socket: %d\n", sock); */
    /* write(sock, "abc", 4); */

    /* exit(0); */

    /* *(char *)(0x1234) = 'a'; */

    /* client = wm_client_connect(175, 600); */
    /* client = wm_client_connect(600, 175); */
    client = wm_client_connect(600, 600);
    /* client = wm_client_connect(100, 100); */
    /* struct wm_client client = wm_client_connect(175, 220); */

    int font_fd = open("/font.psfu", O_RDONLY);
    font = font_parse_fd(font_fd);
    /* pread(0, NULL, 0, 0); */
    /* printf("%d\n", font->header.bytes_per_glyph); */

    if (IS_ERR(font))
    {
        printf("font was invalid, error:%s\n", strerror(GET_ERR(font)));
        exit(1);
    }

    /* memset(framebuffer, 'a', 10); */

    /* int color = 0xFFFF00; */
    /* int color = 0x0000FF; */

    /* memset(w.buffer, 127, win.width * win.height * 4); */
    size_t y2;
    for (y2 = 0; y2 < WM_CLIENT_HEIGHT(client); y2++)
    {
        for (size_t x = 0; x < WM_CLIENT_WIDTH(client); x++)
        {
            client_draw_pixel(client, x, y2, bg_color);
        }
    }

    term = terminal_init(wm_term_operations);

    /* terminal_writechar('a', &term); */

    int stdout_pipe[2];
    pipe(stdout_pipe);

    int stdin_pipe[2];
    pipe(stdin_pipe);

    /* execv("/ls", (char *[]){"ls", NULL}); */
    /* execv("/ksh", (char *[]){"ksh", NULL}); */
    execv_stdio("/bin/ksh", (char *[]){"ksh", NULL}, stdin_pipe[0],
                stdout_pipe[1]);
    /* execv("/bin/ksh", (char *[]){"ksh", NULL}); */

    pipe_write = stdin_pipe[1];
    clone((ptr_t)key_thread);
    /* client_draw_char(client, font, 'a', 0, 0); */
    /* wm_render(client); */

    /* execv_stdio("/ls", (char *[]){"ls", NULL}, 0, 1); */

    /* char *buf = malloc(len); */
    /* read(stdout_pipe[0], buf, len); */
    /* printf("read:%s\n", buf); */

    /* const char *str = "\neno tac"; */
    /* write(stdin_pipe[1], str, strlen(str)); */

    /* read(stdout_pipe[0], buf, len); */
    /* printf("read:%s\n", buf); */

    /* printf("%p-%p\n", client.win.buffer, */
    /*        client.win.buffer + (client.win.h * client.win.w * 4)); */

    x = 1;
    y = 0;
    while (1)
    {

        char *buf = malloc(100);
    read_and_draw_char:;
        bool need_to_read_more = false;
        /* printf("malloc this\n"); */
        size_t res = read(stdout_pipe[0], buf, 100);
        /* size_t res = 0; */

        if (res == 100)
        {
            need_to_read_more = true;
        }

        terminal_add(buf, res, &term);
        if (need_to_read_more)
        {
            goto read_and_draw_char;
        }
        /* logd("rendering"); */
        user_mutex_lock(mutex);
        wm_render(client);
        user_mutex_unlock(mutex);
        /* logd("done rendering"); */
    }

    logd("server disconnected");

    close(client.socket);

    exit(0);
}
