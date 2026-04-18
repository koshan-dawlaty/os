#include "wm_lib.h"
#include <compiler.h>
#include <container_of.h>
#include <err_ptr.h>
#include <extension.h>
#include <fcntl.h>
#include <init.h>
#include <lib/log.h>
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

int pipe(int fds[2])
{
    return syscall_wrapper_asm(SYS_pipe, (ptr)fds, 0, 0, 0, 0);
}

int execv_stdio(const char *path, char *const argv[], int in, int out)
{
    return syscall_wrapper_asm(SYS_execv_stdio, (ptr)path, (ptr)argv, in, out,
                               0);
}

size_t x = 0, y = 0;
struct PSF_font *font;
struct wm_client client;
int pipe_write;
int mutex; /* TODO: maybe have a mutex in the wm_client variable? */

void draw_char(char c)
{

    switch (c)
    {
    case '\n':
        y += font->header.height;
        x = 0;

        break;

    case '\b':

        x -= font->header.width;
        client_draw_rect(client, x, y, font->header.width, font->header.height,
                         0xFF00FF);
        if (x == 0)
        {
            client_draw_rect(client, 0, y, WM_CLIENT_WIDTH(client),
                             font->header.height, 0xFF00FF);
            x = 0;
            y -= font->header.height;
        }

        /* x = 0; */
        /* y = 0; */

        /* size_t y2; */
        /* for (y2 = 0; y2 < WM_CLIENT_HEIGHT(client); y2++) */
        /* { */
        /*     for (size_t x = 0; x < WM_CLIENT_WIDTH(client); x++) */
        /*     { */
        /*         client_draw_pixel(client, x, y2, color); */
        /*     } */
        /* } */
        break;
    default:
        /* printf("%d, %d\n", x, y); */
        client_draw_char(client, font, c, x, y);
        x += font->header.width;
        if (x > WM_CLIENT_WIDTH(client))
        {
            x = 0;
            y += font->header.height;
        }
    }
}

void key_thread()
{
    while (1)
    {
        logd("getting key");
        /* user_mutex_lock(mutex); */
        char c = wm_get_key(client);
        /* user_mutex_unlock(mutex); */
        logd("done getting key");
        if ((u8)c == 0xFF)
        {
            break;
        }
        write(pipe_write, &c, 1);
        printf("%c ", c);
        draw_char(c);
        user_mutex_lock(mutex);
        wm_render(client);
        user_mutex_unlock(mutex);
    }
    loge("bad key");
    exit(1);
}

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
    font = font_parse(font_fd);
    /* printf("%d\n", font->header.bytes_per_glyph); */

    if (IS_ERR(font))
    {
        printf("font was invalid, error:%s\n", strerror(GET_ERR(font)));
        exit(1);
    }

    /* memset(framebuffer, 'a', 10); */

    /* int color = 0xFFFF00; */
    /* int color = 0x0000FF; */

    u32 color = 0x4b3973;
    switch (client.socket)
    {
    case 1:
        color = 0xFF0000;
        break;
    case 2:
        color = 0x00FF00;
        break;
    case 3:
        color = 0xFF;
    }

    /* memset(w.buffer, 127, win.width * win.height * 4); */
    size_t y2;
    for (y2 = 0; y2 < WM_CLIENT_HEIGHT(client); y2++)
    {
        for (size_t x = 0; x < WM_CLIENT_WIDTH(client); x++)
        {
            client_draw_pixel(client, x, y2, color);
        }
    }

    int stdout_pipe[2];
    pipe(stdout_pipe);

    int stdin_pipe[2];
    pipe(stdin_pipe);

    /* execv("/ls", (char *[]){"ls", NULL}); */
    /* execv("/ksh", (char *[]){"ksh", NULL}); */
    printf("execv\n");
    execv_stdio("/ksh", (char *[]){"ksh", NULL}, stdin_pipe[0], stdout_pipe[1]);
    printf("execv end\n");

    pipe_write = stdin_pipe[1];
    clone((ptr)key_thread);
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
    printf("elf_test main loop\n");
    while (1)
    {

        char *buf = malloc(100);
    read_and_draw_char:;
        bool need_to_read_more = false;
        /* printf("malloc this\n"); */
        size_t res = read(stdout_pipe[0], buf, 100);

        if (res == 100)
        {
            need_to_read_more = true;
        }

        for (size_t i = 0; i < res; i++)
        {
            char c = buf[i];
            draw_char(c);
        }
        if (need_to_read_more)
        {
            goto read_and_draw_char;
        }
        logd("rendering");
        user_mutex_lock(mutex);
        wm_render(client);
        user_mutex_unlock(mutex);
        logd("done rendering");
    }

    logd("server disconnected");

    close(client.socket);

    exit(0);
}
