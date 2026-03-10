#include "wm_lib.h"
#include <compiler.h>
#include <err.h>
#include <fcntl.h>
#include <init.h>
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

int sock;
void t1()
{
    char *buf = malloc(4);
    read(sock, buf, 4);
    printf("%s\n", buf);
    exit(0);
}

int main()
{

    init_libc();

    sock = socket_create();

    clone((ptr)t1);
    printf("socket: %d\n", sock);
    write(sock, "abc", 4);

    exit(0);

    /* *(char *)(0x1234) = 'a'; */

    struct wm_client client = wm_client_connect(175, 600);
    /* struct wm_client client = wm_client_connect(175, 220); */

    int font_fd = open("/font.psfu", O_RDONLY);
    struct PSF_font *font = font_parse(font_fd);
    /* printf("%d\n", font->header.bytes_per_glyph); */

    if (IS_ERR(font))
    {
        printf("font was invalid, error:%s\n", strerror(GET_ERR(font)));
        exit(1);
    }

    /* memset(framebuffer, 'a', 10); */

    int color = 0xFFFF00;
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

    size_t x = 1;
    size_t y = 0;
    printf("elf_test main loop\n");
    while (1)
    {

        char *buf = malloc(100);
    read_and_draw_char:;
        bool need_to_read_more = false;
        /* printf("reading from pipe\n"); */
        size_t res = read(stdout_pipe[0], buf, 100);
        if (res == 100)
        {
            need_to_read_more = true;
        }

        /* printf("size:%d\n", res); */

        for (size_t i = 0; i < res; i++)
        {
            char c = buf[i];
            /* terminal_addchar(term, c); */
            switch (c)
            {
            case '\n':
                y += font->header.height;
                x = 0;

                break;

            case '\b':

                x = 0;
                y = 0;

                size_t y2;
                for (y2 = 0; y2 < WM_CLIENT_HEIGHT(client); y2++)
                {
                    for (size_t x = 0; x < WM_CLIENT_WIDTH(client); x++)
                    {
                        client_draw_pixel(client, x, y2, color);
                    }
                }
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
        if (need_to_read_more)
        {
            goto read_and_draw_char;
        }
        wm_render(client);

        char c = wm_get_key(client);
        if (c == '\\')
        {
            break;
        }
        /* const char *str = "\neno tac"; */
        /* const char *str = "cat one"; */
        /* write(stdin_pipe[1], (char[]){'a', 'b'}, 2); */
        /* write(stdin_pipe[1], (char[]){'c', 'd'}, 2); */
        /* printf("%c", c); */
        write(stdin_pipe[1], &c, 1);
    }
    syscall_wrapper_asm(SYS_close_socket, client.socket, 0, 0, 0, 0);

    exit(0);
}
