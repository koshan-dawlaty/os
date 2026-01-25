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

int main(int argc, char **argv)
{
    init_libc();

    char *buf = malloc(10);
    int server_sock;
    recv(0, &server_sock, sizeof(int));
    /* printf("%d\n", server_sock); */

    /* printf("test\n"); */
    struct wm_inital win = {.height = 100, .width = 600};
    send(server_sock, &win, sizeof(struct wm_inital));
    sched_yield();

    struct wm_second second_message;
    recv(server_sock, &second_message, sizeof(int));
    /* printf("client memory:%d\n", second_message.mem_id); */

    void *framebuffer;
    shared_mem_get(second_message.mem_id, &framebuffer);

    /* printf("client buffer:%p\n", framebuffer); */
    /* memset(framebuffer, 'a', 100 * 100 * 4); */

    memset(framebuffer, 255, 100 * 100 * 4);

    for (size_t y = 0; y < win.height; y++)
    {
        int c = y % 2 == 0 ? 127 : 255;
        /* int c = y; */
        memset(framebuffer + y * 4 * win.width, c, win.width * 4);
    }

    struct client_window w = {.w = win.height, win.width, framebuffer};

    for (size_t y = 0; y < win.height; y++)
    {
        for (size_t x = 0; x < win.width; x++)
        {
            if ((x + y) % 2 == 0)
            {
                client_draw_pixel(&w, x, y, 0x0000FF);
            }
            else
            {
                client_draw_pixel(&w, x, y, 0);
            }

            client_draw_pixel(&w, x, y, y);
        }
    }

    int font_fd = open("/font.psfu", O_RDONLY);
    struct PSF_font *font = font_parse(font_fd);

    if (font == NULL)
    {
        /* printf("font was invalid\n"); */
        exit(1);
    }

    /* memset(framebuffer, 'a', 10); */

    int pos = 10;
    size_t x = 1;
    while (1)
    {
        int done_code = WM_RES_RENDER;
        send(server_sock, &done_code, sizeof(int));

        for (size_t i = 0; i < 1; i++)
        {
            client_draw_pixel(&w, pos, 1, 0xFF0000);
            pos++;
        }

        /*         if (server_sock == 1) */
        /*         { */

        /*             /1* int key_code = WM_RES_KEY; *1/ */
        /*             /1* send(server_sock, &key_code, sizeof(int)); *1/ */
        /*             /1* char c; *1/ */
        /*             /1* recv(server_sock, &c, sizeof(char)); *1/ */

        /*         } */
        client_draw_char(&w, font, getchar(), x, 20);

        for (size_t i = 0; i < 1; i++)
        {
            sched_yield();
        }

        /* client_draw_char(&w, font, 'a', x, 20); */
        /* client_draw_char(&w, font, getchar(), x, 20); */
        x += font->header.width;
    }

    exit(0);
}
