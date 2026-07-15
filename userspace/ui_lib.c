#include "lib/img.h"
#include "unistd.h"
#include "wm_lib.h"
#include <container_of.h>
#include <init.h>
#include <lib/assert.h>
#include <lib/font.h>
#include <lib/framebuffer.h>
#include <lib/list.h>
#include <lib/log.h>
#include <lib/rect.h>
#include <string.h>
#include <userspace/mouse.h>

struct wm_client client;
struct PSF_font *font;

enum view_type
{
    VIEW_LAYOUT,
    VIEW_ELEMENT,
};

struct view
{
    size_t x, y;

    struct framebuffer fb;
    struct list list;
    union
    {
        struct
        {
            struct layout *layout;
        };
        struct element *element;
    };
    enum view_type type;
};
struct element_type
{
    void (*render)(struct view *e);
    void (*on_click)(struct view *e, size_t real_x, size_t real_y);
    void (*free)(struct view *e);
};

struct element
{
    struct element_type type;
    void *private;
};

static void image_render(struct view *e)
{
    struct image *img = (struct image *)e->element->private;
    e->fb.height = img->header.h;
    e->fb.width = img->header.w;
    framebuffer_draw_img(&e->fb, 0, 0, *img);
}

struct element_type element_image = {.render = image_render};

static void rect_render(struct view *e)
{
    logd("%p", e->element->private);
    framebuffer_draw_rect(&e->fb, 0, 0, e->fb.width, e->fb.height,
                          (ptr_t)e->element->private);
}

struct element_type element_rect = {.render = rect_render};

struct element *element_alloc(struct element_type type, void *private)
{
    struct element *out = malloc(sizeof(struct element));
    out->type = type;
    out->private = private;
    return out;
}

struct layout_type
{
    void (*render_children)(struct view *);
};

struct layout
{
    struct layout_type type;
    struct list children;
    void *private;
};

void view_render(struct view *view);

void line_layout_render_children(struct view *view)
{
    size_t p = 0;
    struct list *pos;
    list_for_each(pos, &view->layout->children)
    {
        struct view *child = containerof(pos, struct view, list);
        if (view->layout->private != 0)
        {
            child->x = p;
            child->y = 0;
        }
        else
        {
            child->x = 0;
            child->y = p;
        }
        child->fb.pitch = view->fb.pitch;
        child->fb.depth = view->fb.depth;
        child->fb.pixel_width = view->fb.pixel_width;
        child->fb.buffer =
            view->fb.buffer + FRAMEBUFFER_OFFSET(view->fb, child->x, child->y);

        logd("%d, %d", child->fb.width, child->fb.height);

        view_render(child);
        if (view->layout->private != 0)
        {
            p += child->fb.width + 10;
        }
        else
        {
            p += child->fb.height + 10;
        }
    }
}

struct layout_type line_layout = {.render_children =
                                      line_layout_render_children};

struct layout *layout_alloc(struct layout_type type, void *private)
{
    struct layout *out = malloc(sizeof(struct layout));
    out->type = type;
    out->private = private;
    list_init_head(&out->children);
    return out;
}

static inline struct view *view_alloc_partial(enum view_type type)
{
    struct view *out = malloc(sizeof(struct view));
    out->list = LIST_INIT();
    out->x = 0;
    out->y = 0;
    out->fb.width = 10;
    out->fb.height = 10;
    out->type = type;

    return out;
}
static inline struct view *view_alloc_element(struct element *element)
{
    struct view *out = view_alloc_partial(VIEW_ELEMENT);
    out->element = element;
    return out;
}
static inline struct view *view_alloc_layout(struct layout *layout)
{
    struct view *out = view_alloc_partial(VIEW_LAYOUT);
    out->layout = layout;
    return out;
}

struct view *ui_rect_alloc(size_t w, size_t h, u32 color)
{
    struct view *out =
        view_alloc_element(element_alloc(element_rect, (void *)(ptr_t)color));
    out->fb.width = w;
    out->fb.height = h;
    return out;
}

struct view *ui_image_alloc(struct image *img)
{
    struct view *out = view_alloc_element(element_alloc(element_image, img));
    out->fb.width = img->header.w;
    out->fb.height = img->header.h;

    return out;
}

static inline struct view *layout_line_horizontal_alloc()
{
    return view_alloc_layout(layout_alloc(line_layout, (void *)1));
}
static inline struct view *layout_line_vertical_alloc()
{
    return view_alloc_layout(layout_alloc(line_layout, (void *)0));
}

void layout_add_child(struct view *layout, struct view *child)
{
    ASSERT(layout->type == VIEW_LAYOUT, "layout was not a layout");

    if (child->fb.width > layout->fb.width)
    {
        layout->fb.width = child->fb.width;
    }
    if (child->fb.height > layout->fb.height)
    {
        layout->fb.height = child->fb.height;
    }

    list_push(&child->list, &layout->layout->children);
}

struct view *ui_image_alloc_str(const char *path)
{
    struct image *img = malloc(sizeof(struct image));
    image_parse(path, img);

    return ui_image_alloc(img);
}

void view_render(struct view *view)
{
    if (view->type == VIEW_ELEMENT)
    {
        view->element->type.render(view);
    }
    else if (view->type == VIEW_LAYOUT)
    {
        view->layout->type.render_children(view);
    }
    else
    {
        PANIC("unknown enum type:%d", view->type);
    }
}

int main()
{

    client = wm_client_connect(600, 600);

    int font_fd = open("/font.psfu", O_RDONLY);
    font = font_parse_fd(font_fd);

    struct view *image = ui_image_alloc_str("dog.tga");
    struct view *osdev = ui_image_alloc_str("osdev.tga");

    struct view *blue = ui_rect_alloc(10, 10, 0xFF);
    struct view *purple = ui_rect_alloc(10, 10, 0xFF00FF);
    struct view *green = ui_rect_alloc(10, 10, 0xFF00);
    struct view *cyan = ui_rect_alloc(10, 10, 0xFFFF);

    struct view *horizontal = layout_line_horizontal_alloc();
    struct view *vertical = layout_line_vertical_alloc();

    layout_add_child(horizontal, image);
    layout_add_child(horizontal, purple);
    layout_add_child(horizontal, vertical);

    layout_add_child(vertical, green);
    layout_add_child(vertical, blue);

    horizontal->fb = client.fb;

    view_render(horizontal);

    while (true)
    {
        wm_render(client);
        struct wm_message message;
        wm_get_message(&client, &message);
        switch (message.header.type)
        {
        case WM_MESSAGE_MOUSE:;
            struct mouse_event *ev = message.message_data;

            break;
        }
    }
}
