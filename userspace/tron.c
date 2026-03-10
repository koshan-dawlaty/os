#include "sched.h"
#include "sys/types.h"
#include "wm_lib.h"
#include <fcntl.h>
#include <init.h>
#include <lib/framebuffer.h>
#include <stdio.h>
#include <stdlib.h>
#include <types.h>
#include <unistd.h>

enum player_dir
{
    DIR_UP,
    DIR_LEFT,
    DIR_DOWN,
    DIR_RIGHT,
};

int main()
{

    init_libc();

    size_t w = 100, h = 100;
    size_t x = 0, y = 0;
    struct wm_client client = wm_client_connect_nonblock(h, w);
    enum player_dir dir = DIR_DOWN;

    while (1)
    {
        wm_render(client);
        char c = wm_get_key_nonblock(client);
        if (c != 0)
        {
            switch (c)
            {
            case 'k':
                if (dir != DIR_DOWN)
                {
                    dir = DIR_UP;
                }
                break;
            case 'l':
                if (dir != DIR_RIGHT)
                {
                    dir = DIR_LEFT;
                }
                break;
            case 'j':
                if (dir != DIR_UP)
                {
                    dir = DIR_DOWN;
                }
                break;
            case 'h':
                if (dir != DIR_LEFT)
                {
                    dir = DIR_RIGHT;
                }
                break;
            }
            if (c == 'q')
            {
                break;
            }
        }
        switch (dir)
        {
        case DIR_UP:
            y--;
            break;
        case DIR_LEFT:
            x++;
            break;
        case DIR_DOWN:
            y++;
            break;
        case DIR_RIGHT:
            x--;
            break;
        }

        if (x <= 1)
        {
            x = 1;
        }
        else if (x >= w)
        {
            x = w;
        }
        if (y <= 1)
        {
            y = 1;
        }
        else if (y >= h)
        {
            y = h;
        }

        if (framebuffer_get_pixel(&client.fb, x, y) != 0)
        {
            framebuffer_draw_rect(&client.fb, 0, 0, w, h, 0);
            x = 0;
            y = 0;
        }

        client_draw_pixel(client, x, y, 0xFFFFFF);
        sleep(1);
    }

    syscall_wrapper_asm(SYS_close_socket, client.socket, 0, 0, 0, 0);

    exit(0);
}
