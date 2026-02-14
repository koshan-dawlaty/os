#include "wm_lib.h"
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

int main()
{
    init_libc();

    struct wm_client client = wm_client_connect();

    int font_fd = open("/font.psfu", O_RDONLY);
    struct PSF_font *font = font_parse(font_fd);
    /* printf("%d\n", font->header.bytes_per_glyph); */

    if (font == NULL)
    {
        /* printf("font was invalid\n"); */
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
    size_t y;
    for (y = 0; y < client.win.h; y++)
    {
        for (size_t x = 0; x < client.win.w; x++)
        {
            client_draw_pixel(&client.win, x, y, color);
        }
    }

    int p[2];
    pipe(p);
    /* execv("/ls", (char *[]){"ls", NULL}); */
    execv_stdio("/ls", (char *[]){"ls", NULL}, 0, p[1]);
    /* execv_stdio("/ls", (char *[]){"ls", NULL}, 0, 1); */
    char *buf = malloc(10);
    read(p[0], buf, 10);
    printf("read:%s\n", buf);

    int pos = 10;
    size_t x = 1;
    while (1)
    {
        wm_render(client);

        char c = wm_get_key(client);
        client_draw_char(&client.win, font, c, x, 50);
        x += font->header.width;
    }

    exit(0);
}
