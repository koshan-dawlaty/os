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
typedef uintptr_t ptr;
extern int syscall_wrapper_asm(ptr ax, ptr bx, ptr cx, ptr dx, ptr si, ptr di);

struct framebuffer *framebuffer_alloc()
{
    struct framebuffer *out = NULL;
    syscall_wrapper_asm(18, (ptr)&out, 0, 0, 0, 0);

    return out;
}

int socket_create()
{
    return syscall_wrapper_asm(SYS_create_socket, 0, 0, 0, 0, 0);
}

int send(int socket, void *buf, size_t len)
{
    return syscall_wrapper_asm(SYS_send, socket, (ptr)buf, len, 0, 0);
}

int recv(int socket, void *buf, size_t len)
{
    return syscall_wrapper_asm(SYS_recv, socket, (ptr)buf, len, 0, 0);
}

int shared_mem_alloc(void **addr, size_t len)
{
    return syscall_wrapper_asm(SYS_shared_mem_alloc, (ptr)addr, len, 0, 0, 0);
}

int shared_mem_get(int id, void **p)
{
    return syscall_wrapper_asm(SYS_shared_mem_get, id, (ptr)p, 0, 0, 0);
}

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
    printf("client memory:%d\n", second_message.mem_id);

    void *framebuffer;
    shared_mem_get(second_message.mem_id, &framebuffer);

    printf("client buffer:%p\n", framebuffer);
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
        printf("font was invalid\n");
        exit(1);
    }

    /* memset(framebuffer, 'a', 10); */

    int pos = 10;
    size_t x = 10;
    while (1)
    {
        int done_code = WM_RES_RENDER;
        send(server_sock, &done_code, sizeof(int));
        pos++;
        client_draw_pixel(&w, pos, pos, 0x00FF00);

        client_draw_char(&w, font, getchar(), x, 20);
        x += font->header.width;
    }

    exit(0);
}
