#include <container_of.h>
#include <init.h>
#include <lib/array.h>
#include <lib/assert.h>
#include <lib/font.h>
#include <lib/framebuffer.h>
#include <lib/img.h>
#include <lib/list.h>
#include <lib/log.h>
#include <lib/rect.h>
#include <lib/ui/ui.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <userspace/mouse.h>

static void image_render(struct view *e)
{
    struct image *img = (struct image *)e->element->private;
    e->fb.height = img->header.h;
    e->fb.width = img->header.w;
    framebuffer_draw_img(&e->fb, 0, 0, *img);
}

struct element_type element_image = {.render = image_render, .on_click = NULL};

static void rect_render(struct view *e)
{
    logd("%p", e->element->private);
    framebuffer_draw_rect(&e->fb, 0, 0, e->fb.width, e->fb.height,
                          (ptr_t)e->element->private);
}

void line_layout_render_children(struct view *view)
{
    struct line_layout *layout = view->layout->private;
    size_t p = 0;
    struct list *pos;
    list_for_each(pos, &view->layout->children)
    {
        struct view *child = containerof(pos, struct view, list);
        if (!layout->is_vertical)
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

        view_render(child);
        if (!layout->is_vertical)
        {
            p += child->fb.width + layout->offset;
        }
        else
        {
            p += child->fb.height + layout->offset;
        }
    }
}

struct layout_type line_layout = {.render_children =
                                      line_layout_render_children};

struct element_type element_rect = {.render = rect_render, .on_click = NULL};
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

struct line_layout *line_layout_alloc(bool is_vertical, size_t offset)
{
    struct line_layout *out = malloc(sizeof(struct line_layout));
    out->is_vertical = is_vertical;
    out->offset = offset;
    return out;
}

struct view *layout_line_horizontal_alloc(size_t offset)
{
    return view_alloc_layout(
        layout_alloc(line_layout, line_layout_alloc(false, offset)));
}
struct view *layout_line_vertical_alloc(size_t offset)
{
    return view_alloc_layout(
        layout_alloc(line_layout, line_layout_alloc(true, offset)));
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

struct button
{
    struct view *child;
};

static void button_render(struct view *e)
{
    framebuffer_draw_rect(&e->fb, 0, 0, e->fb.width, e->fb.height, 0xFF00FF);
    struct button *button = e->element->private;
    button->child->fb.buffer = e->fb.buffer + FRAMEBUFFER_OFFSET(e->fb, 5, 5);
    button->child->fb.pitch = e->fb.pitch;
    button->child->fb.depth = e->fb.depth;
    button->child->fb.pixel_width = e->fb.pixel_width;
    /* button->child->fb.height */

    view_render(button->child);
}

static void button_onclick(struct view *e, size_t real_x, size_t real_y)
{

    framebuffer_draw_rect(&e->fb, 0, 0, e->fb.width, e->fb.height, 0xAA00AA);
}

struct element_type button_type = {
    .render = button_render,
    .on_click = button_onclick,
};

struct view *button_alloc()
{
    struct button *button = malloc(sizeof(struct button));

    struct view *out = view_alloc_element(element_alloc(button_type, button));
    out->fb.width = 100;
    out->fb.height = 20;
    return out;
}

struct view *button_alloc_child(struct view *child)
{
    struct view *out = button_alloc();
    struct button *button = out->element->private;
    button->child = child;
    out->fb.height = child->fb.height;
    out->fb.width = child->fb.width;
    return out;
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

void view_onclick(struct view *view, size_t click_x, size_t click_y,
                  size_t x_abs, size_t y_abs)
{
    ASSERT(view->type == VIEW_LAYOUT,
           "onclick should only get called on a layout, it should proccess the "
           "click if it is an element");

    struct list *pos;
    list_for_each(pos, &view->layout->children)
    {
        struct view *child = containerof(pos, struct view, list);
        if (child->type == VIEW_LAYOUT)
        {
            view_onclick(child, click_x, click_y, child->x + x_abs,
                         child->y + y_abs);
        }
        else if (child->type == VIEW_ELEMENT)
        {
            size_t x_pos = child->x + x_abs;
            size_t y_pos = child->y + y_abs;
            if (click_x > x_pos && click_x < x_pos + child->fb.width &&
                click_y > y_pos && click_y < y_pos + child->fb.height)
            {
                if (child->element->type.on_click != NULL)
                {
                    child->element->type.on_click(child, click_x, click_y);
                }
            }
        }
        else
        {
            PANIC("unknown enum type %d", child->type);
        }
    }
}
