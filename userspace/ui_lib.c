#include "lib/list.h"
#include "lib/log.h"
#include "wm_lib.h"
#include <init.h>

struct rect_d
{
    double x, y, w, h;
};
#define RECT_D_INIT(_x, _y, _w, _h)                                            \
    ((struct rect_d){.x = _x, .y = _y, .w = _w, .h = _h})

struct wm_client client;

struct rect_d get_real_pos(struct rect_d parent, struct rect_d child)
{

    double x = (child.x * parent.w) + parent.x;
    double y = (child.y * parent.h) + parent.y;
    double width = child.w * parent.w;
    double height = child.h * parent.h;

    return (struct rect_d){.x = x * client.fb.width,
                           .y = y * client.fb.height,
                           .w = width * client.fb.width,
                           .h = height * client.fb.height};
}

struct element
{
    struct rect_d r;
    u32 color;
    struct list2c list;
    struct list2c children;
};

struct element *element_alloc(struct rect_d r, u32 color)
{
    struct element *out = malloc(sizeof(struct element));
    out->color = color;
    out->children = LIST2C_HEAD_INIT(&out->children);
    out->list = LIST2C_INIT();
    out->r = r;
    return out;
}

void render(struct element *e, struct rect_d parent)
{

    struct rect_d screen = get_real_pos(parent, e->r);
    client_draw_rect(client, screen.x, screen.y, screen.w, screen.h, e->color);

    struct rect_d r = e->r;
    double x = parent.x + (r.x * parent.w);
    double y = parent.y + (r.y * parent.h);
    double height = parent.h * r.h;
    double width = parent.w * r.w;

    struct rect_d new_rect = RECT_D_INIT(x, y, width, height);

    struct list2c *pos;
    list2c_for_each(pos, &e->children)
    {
        struct element *e = containerof(pos, struct element, list);
        render(e, new_rect);
    }
}

int main()
{

    init_libc();

    client = wm_client_connect(600, 600);
    int font_fd = open("/font.psfu", O_RDONLY);
    struct PSF_font *font = font_parse(font_fd);

    struct element *root = element_alloc(RECT_D_INIT(0, 0, 1, 1), 0xFF00FF);
    struct element *one = element_alloc(RECT_D_INIT(0.5, 0, 1, 0.5), 0xFF0000);
    struct element *two = element_alloc(RECT_D_INIT(0.5, 0, 0.5, 1), 0xFFFF00);

    list2c_push(&one->list, &root->children);
    list2c_push(&two->list, &one->children);

    render(root, RECT_D_INIT(0, 0, 1, 1));

    while (true)
    {
        wm_render(client);
        struct wm_message message;
        wm_get_message(client, &message);
    }
}
