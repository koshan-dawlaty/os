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

    struct client_window w = {
        .w = win.width, .h = win.height, .buffer = framebuffer};

    int font_fd = open("/font.psfu", O_RDONLY);
    struct PSF_font *font = font_parse(font_fd);
    printf("%d\n", font->header.bytes_per_glyph);
    
    struct wm_client client = {.win = w, .socket = server_sock};

    if (font == NULL)
    {
        /* printf("font was invalid\n"); */
        exit(1);
    }

    /* memset(framebuffer, 'a', 10); */

    int color = 0xFFFF00;
    switch (server_sock)
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
    for (y = 0; y < win.height; y++)
    {
        for (size_t x = 0; x < win.width; x++)
        {
            client_draw_pixel(&w, x, y, color);
        }
    }

    int pos = 10;
    size_t x = 1;
    while (1)
    {
        /* printf("elf_test is rendering\n"); */
        wm_render(client);

        /* client_draw_pixel(&w, pos, 0, color); */
        /* pos++; */

        /* TODO: FIXME: TODO: FIXME: this is a temporary fix so that */

        int key_code = WM_RES_KEY;
        send(server_sock, &key_code, sizeof(int));

        char c;
        recv(server_sock, &c, sizeof(char));

        client_draw_char(&w, font, c, x, 50);
        x += font->header.width;
    }

    exit(0);
}
