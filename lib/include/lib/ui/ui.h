#ifndef UI_H
#define UI_H

#include <lib/framebuffer.h>
#include <lib/img.h>
#include <lib/list.h>
#include <stdlib.h>
#include <types.h>

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

struct view_array
{
    struct view **items;
    size_t capacity;
    size_t num;
};
struct element_type
{
    void (*render)(struct view *e);
    void (*on_click)(struct view *e, size_t real_x, size_t real_y);
    void (*free)(struct view *e);
};
void view_render(struct view *view);

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

struct line_layout
{
    bool is_vertical;
    size_t offset;
};

struct element
{
    struct element_type type;
    void *private;
};

static inline struct layout *layout_alloc(struct layout_type type,
                                          void *private)
{
    struct layout *out = malloc(sizeof(struct layout));
    out->type = type;
    out->private = private;
    list_init_head(&out->children);
    return out;
}

static inline struct element *element_alloc(struct element_type type,
                                            void *private)
{
    struct element *out = malloc(sizeof(struct element));
    out->type = type;
    out->private = private;
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

struct view *ui_rect_alloc(size_t w, size_t h, u32 color);
struct view *ui_image_alloc(struct image *img);

void layout_add_child(struct view *layout, struct view *child);
struct view *button_alloc();
struct view *button_alloc_child(struct view *child);
void view_render(struct view *view);
struct view *layout_line_vertical_alloc(size_t offset);
struct view *layout_line_horizontal_alloc(size_t offset);
struct view *ui_image_alloc(struct image *img);
struct view *ui_image_alloc_str(const char *path);
void view_onclick(struct view *view, size_t click_x, size_t click_y,
                  size_t x_abs, size_t y_abs);

#endif
