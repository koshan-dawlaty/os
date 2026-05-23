#include "lib/assert.h"
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
struct PSF_font *font;

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

struct element;
struct element_type
{
    void (*render)(struct element *e, struct rect_d parent);
};
struct element
{
    struct rect_d r;
    struct list2c list;
    struct list2c children;

    struct element_type type;
    void *private;
};

void render_text()
{
}

struct element *element_alloc(struct rect_d r, const struct element_type t,
                              void *element_private)
{
    struct element *out = malloc(sizeof(struct element));
    out->children = LIST2C_HEAD_INIT(&out->children);
    out->list = LIST2C_INIT();
    out->r = r;
    out->type = t;
    out->private = element_private;
    return out;
}

void render(struct element *e, struct rect_d parent)
{

    /* struct rect_d screen = get_real_pos(parent, e->r); */
    NOT_NULL(e->type.render, "element did not implement render operation");
    e->type.render(e, parent);
    /* client_draw_rect(client, screen.x, screen.y, screen.w, screen.h,
     * e->color); */

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

void rect_render(struct element *rect, struct rect_d parent)
{
    struct rect_d r = get_real_pos(parent, rect->r);
    client_draw_rect(client, r.x, r.y, r.w, r.h, (ptr)rect->private);
}
const struct element_type rect_operations = {.render = rect_render};

struct element *rectangle_alloc(struct rect_d r, u32 color)
{
    return element_alloc(r, rect_operations, (void *)(ptr)color);
}

void text_render(struct element *rect, struct rect_d parent)
{
    struct rect_d r = get_real_pos(parent, rect->r);
    /* TODO: use width and height maybe */
    client_draw_string(client, font, rect->private, r.x, r.y);
}
const struct element_type text_operations = {.render = text_render};

struct element *text_alloc(struct rect_d r, char *str)
{
    return element_alloc(r, text_operations, strdup(str));
}

int main()
{

    init_libc();

    client = wm_client_connect(600, 600);
    int font_fd = open("/font.psfu", O_RDONLY);
    font = font_parse(font_fd);

    struct element *root =
        rectangle_alloc(RECT_D_INIT(0.0, 0.0, 1.0, 1.0), 0x00000);
    struct element *one =
        rectangle_alloc(RECT_D_INIT(0.5, 0.0, 0.5, 1), 0xFF0000);
    struct element *two =
        rectangle_alloc(RECT_D_INIT(0.5, 0.0, 0.5, 1.0), 0x0000FF);
    struct element *three = text_alloc(RECT_D_INIT(0.5, 0, 0.5, 1), "abc");

    list2c_push(&one->list, &root->children);
    list2c_push(&two->list, &one->children);
    list2c_push(&three->list, &two->children);

    render(root, RECT_D_INIT(0, 0, 1, 1));

    while (true)
    {
        wm_render(client);
        struct wm_message message;
        wm_get_message(client, &message);
    }
}
