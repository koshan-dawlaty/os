#include "container_of.h"
#include "lib/framebuffer.h"
#include "userspace/mouse.h"
#include "wm_lib.h"
#include <init.h>
#include <lib/assert.h>
#include <lib/list.h>
#include <lib/log.h>

struct rect_d
{
    double x, y, w, h;
};
#define RECT_D_INIT(_x, _y, _w, _h)                                            \
    ((struct rect_d){.x = _x, .y = _y, .w = _w, .h = _h})

struct wm_client client;
struct PSF_font *font;

struct rect_d convert_screen_pos(struct rect_d parent, struct rect_d child)
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
    void (*render)(struct element *e);
    void (*on_click)(struct element *e, size_t real_x, size_t real_y);
    void (*free)(struct element *e);
};
struct element
{
    size_t x, y;
    size_t abs_x, abs_y; /* position of element in the screen framebuffer */
    struct framebuffer fb;
    struct list2c list;
    struct list2c children;

    struct element_type type;
    struct element *parent;
    void *private;
};

static inline struct element *element_alloc_partial(struct element_type t,
                                                    void *private)
{
    struct element *out = malloc(sizeof(struct element));
    out->children = LIST2C_HEAD_INIT(&out->children);
    out->list = LIST2C_INIT();

    out->type = t;
    out->private = private;

    return out;
}

struct element *element_alloc_root(const struct element_type t,
                                   void *element_private,
                                   struct framebuffer screen)
{
    struct element *out = element_alloc_partial(t, element_private);
    out->x = 0;
    out->y = 0;
    out->abs_x = 0;
    out->abs_y = 0;

    out->fb = screen;

    return out;
}

struct element *element_alloc(struct rect_d r, const struct element_type t,
                              void *element_private, struct element *parent)
{

    struct element *out = element_alloc_partial(t, element_private);

    if (parent != NULL)
    {

        list2c_push(&out->list, &parent->children);

        out->abs_x = (r.x * parent->fb.width) + parent->abs_x;
        out->abs_y = (r.y * parent->fb.height) + parent->abs_y;
        out->x = (r.x * parent->fb.width);
        out->y = (r.y * parent->fb.height);

        logd("%d, %d", out->x, out->y);

        size_t w = r.w * parent->fb.width;
        size_t h = r.h * parent->fb.height;
        size_t pitch = (parent->fb.pitch / parent->fb.pixel_width) - w;
        if (unlikely(parent->fb.pitch / parent->fb.pixel_width == w))
        {
            pitch = parent->fb.pitch / parent->fb.pixel_width;
        }

        void *buf =
            parent->fb.buffer + FRAMEBUFFER_OFFSET(parent->fb, out->x, out->y);

        out->fb = FRAMEBUFFER_INIT(w, h, parent->fb.pitch, parent->fb.depth,
                                   parent->fb.pixel_width, buf);
    }

    return out;
}

void render(struct element *e)
{

    /* struct rect_d screen = get_real_pos(parent, e->r); */
    NOT_NULL(e->type.render, "element did not implement render operation");
    e->type.render(e);
    /* client_draw_rect(client, screen.x, screen.y, screen.w, screen.h,
     * e->color); */

    struct list2c *pos;
    list2c_for_each(pos, &e->children)
    {
        struct element *e2 = containerof(pos, struct element, list);
        render(e2);
    }
}

void rect_free(struct element *e)
{
}

void rect_onclick(struct element *e, size_t real_x, size_t real_y)
{
    u32 color = (ptr)e->private;
    logd("%p", color);
    /* color <<= 8; */
    client_draw_pixel(client, real_x, real_y, 0);
}

void rect_render(struct element *rect)
{
    framebuffer_draw_rect(&rect->fb, 0, 0, rect->fb.width, rect->fb.height,
                          (ptr)rect->private);
}
const struct element_type rect_operations = {

    .render = rect_render,

    .free = rect_free,
    .on_click = rect_onclick

};

struct element *rectangle_alloc(struct rect_d r, struct element *parent,
                                u32 color)
{
    if (parent == NULL) /* TODO: stupid */
    {
        return element_alloc_root(rect_operations, (void *)(ptr)color,
                                  client.fb);
    }
    else
    {
        return element_alloc(r, rect_operations, (void *)(ptr)color, parent);
    }
}

void ui_onclick(
    struct element *root, size_t x,
    size_t y) /* TODO: SLOW: this would be faster if it only ran on_click on
                 ones we knew were in the right position */
{

    logd("mouse:%d, %d, color(%p) %d, %d", x, y, (ptr)root->private, root->x,
         root->y);
    if (x > root->abs_x && x < root->fb.width + root->abs_x &&
        y > root->abs_y && y < root->abs_y + root->fb.height)
    {
        logd("click on:%p", (ptr)root->private);
        root->type.on_click(root, x, y);
    }

    struct list2c *pos;
    list2c_for_each(pos, &root->children)
    {
        struct element *e = containerof(pos, struct element, list);
        ui_onclick(e, x, y);
    }
}

/* void text_free(struct element *e) */
/* { */
/*     free(e->private); */
/* } */

/* void text_render(struct element *rect, struct rect_d parent) */
/* { */
/*     struct rect_d r = convert_screen_pos(parent, rect->r); */
/*     /1* TODO: use width and height maybe *1/ */
/*     client_draw_string(client, font, rect->private, r.x, r.y); */
/* } */
/* const struct element_type text_operations = {.render = text_render, */
/*                                              .free = text_free}; */

/* struct element *text_alloc(struct rect_d r, char *str) */
/* { */
/*     return element_alloc(r, text_operations, strdup(str)); */
/* } */

int main()
{

    init_libc();

    client = wm_client_connect(600, 600);
    logd("%d", client.fb.pitch);
    int font_fd = open("/font.psfu", O_RDONLY);
    font = font_parse(font_fd);

    struct element *root =
        rectangle_alloc(RECT_D_INIT(0.0, 0.0, 1.0, 1.0), NULL, 0xFF0000);
    struct element *one =
        rectangle_alloc(RECT_D_INIT(0.25, 0.5, 0.25, 0.25), root, 0x0000FF);
    struct element *two =
        rectangle_alloc(RECT_D_INIT(0.5, 0.0, 0.5, 1.0), one, 0xFF00FF);
    struct element *three =
        rectangle_alloc(RECT_D_INIT(0.5, 0.0, 0.5, 1.0), two, 0xFFFFFF);
    /* struct element *three = text_alloc(RECT_D_INIT(0.5, 0.0, 0.5, 1), "abc");
     */

    /* list2c_push(&three->list, &two->children); */

    int mouse_x = 0, mouse_y = 0;

    render(root);

    while (true)
    {
        wm_render(client);
        struct wm_message message;
        wm_get_message(client, &message);
        switch (message.header.type)
        {
        case WM_MESSAGE_MOUSE:;
            struct mouse_event *ev = message.message_data;
            /* logd("%d, %d", ev.x, mouse_y); */
            mouse_x += ev->x_change;
            mouse_y += ev->y_change;
            /* logd("mouse_x:%d, %d", mouse_x, mouse_y); */
            /* client_draw_pixel(client, mouse_x, mouse_y, 0xFFFFFF); */
            if (ev->buttons.left_click)
            {
                ui_onclick(root, mouse_x, mouse_y);
            }
            break;
        }
    }
}
