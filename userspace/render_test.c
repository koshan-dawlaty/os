#include "lib/framebuffer.h"
#include "lib/log.h"
#include "wm_lib.h"
#include <init.h>
#include <stdio.h>
#include <stdlib.h>

struct point2d
{
    double x, y;
};

struct point3d
{
    double x, y, z;
};

struct screen_point
{
    size_t x, y;
};
struct wm_client client;

int screen_height()
{
    return client.fb.height;
}

double x87_sin(double x)
{
    double result;
    asm("fsin" : "=t"(result) : "0"(x));
    return result;
}

double x87_cos(double x)
{
    double result;
    asm("fcos" : "=t"(result) : "0"(x));
    return result;
}

struct point3d rotate_xz(struct point3d p, double angle)
{

    double s = x87_sin(angle);
    double c = x87_cos(angle);

    return (struct point3d){
        .x = (p.x * c) - (p.z * s), .z = (p.x * s) + (p.z * c), .y = p.y};
}

int screen_width()
{
    return client.fb.width;
}

struct point2d to_point2d(struct point3d p)
{
    return (struct point2d){p.x / p.z, p.y / p.z};
}

struct screen_point to_screen_point(struct point2d p)
{
    int width = screen_width();
    int height = screen_height();
    int x = (p.x + 1) / 2 * width;
    int y = (1 - (p.y + 1) / 2) * height;
    return (struct screen_point){x, y};
}

void draw_point(struct screen_point p)
{
    size_t s = 10;
    client_draw_rect(client, p.x - s / 2, p.y - s / 2, s, s, 0xFF00);
}

// clang-format off
struct point3d points[] = {
    {0.5, 0.5, 3},
    {0.5, -.5, 3},
    {-.5, -.5, 3},
    {-.5, 0.5, 3},

    {0.5, 0.5, 2},
    {0.5, -.5, 2},
    {-.5, -.5, 2},
    {-.5, 0.5, 2},
};

int faces[][4]  = {
	{0, 1, 2, 3},
	{4, 5, 6, 7},
};

// clang-format on

static inline struct screen_point convert_real_point(struct point3d p)
{
    return to_screen_point(to_point2d(p));
}

void clear()
{
    client_draw_rect(client, 0, 0, screen_width(), screen_height(), 0);
}

void draw_line(struct point3d one, struct point3d two)
{
    struct screen_point p1 = convert_real_point(one);
    struct screen_point p2 = convert_real_point(two);

    framebuffer_draw_line(&client.fb, p1.x, p1.y, p2.x, p2.y, 0xFFFF00);
}

int main()
{

    size_t w = 300, h = 300;
    size_t x = 0, y = 0;
    client = wm_client_connect(h, w);

    /*
draw_point(to_screen_point((struct point2d){.x = 0.5, .y = 0.5}));
draw_point(to_screen_point((struct point2d){.x = -0.5, .y = -0.5}));
draw_point(to_screen_point((struct point2d){.x = -0.5, .y = 0.5}));
draw_point(to_screen_point((struct point2d){.x = 0.5, .y = -0.5}));
*/

    /* __builtin_sqrt(); */

    double z = 0;
    double angle = 0;

    size_t x2 = 0;
    while (1)
    {
        /* x2++; */

        /* framebuffer_draw_line(&client.fb, 100 + x2, 30, 1, 1, 0xFF00FF); */
        /* wm_render(client); */
        /* wm_get_key(client); */
        /* clear(); */
        /* continue; */

        for (size_t i = 0; i < sizeof(points) / sizeof(struct point3d); i++)
        {

            points[i] = rotate_xz(points[i], angle);
            /* points[i] = rotate_xz(points[i], 0.000000001); */
            points[i].z += z;
            struct screen_point point = to_screen_point(to_point2d(points[i]));
            draw_point(point);
        }

        draw_line(points[7], points[3]);
        draw_line(points[6], points[2]);
        draw_line(points[5], points[1]);
        draw_line(points[4], points[0]);

        for (size_t i = 0; i < sizeof(faces) / sizeof(int[4]); i++)
        {
            int *face = faces[i];

            for (size_t j = 1; j < 5; j++)
            {
                struct point3d p1 = points[face[j - 1]];
                struct point3d p2 = points[face[j % 4]];
                struct screen_point prev =
                    convert_real_point(points[face[j - 1]]);
                struct screen_point point =
                    convert_real_point(points[face[j % 4]]);
                draw_line(points[face[j - 1]], points[face[j % 4]]);
            }
        }

        wm_render(client);
        struct wm_message message;
        wm_get_message(&client, &message);
        if (message.header.type == WM_MESSAGE_KEY)
        {
            char c = *(char *)message.message_data;
            /* char c = 'a'; */
            if (c == 'a')
            {
                angle = 0.1;
            }
            else if (c == 's')
            {
                angle = -0.1;
            }
            else
            {
                angle = 0;
            }

            if (c == 'd')
            {
                z = 0.1;
            }
            else if (c == 'f')
            {
                z = -0.1;
            }
            else
            {
                z = 0;
            }
        }
        else
        {
            angle = 0;
            z = 0;
        }
        clear();
    }

    exit(0);
}
