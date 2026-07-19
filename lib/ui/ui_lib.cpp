#include <cpp/cpp_c_include_start.hpp>

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
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <userspace/mouse.h>

#include <cpp/cpp_c_include_end.hpp>

#include <lib/ui/ui.hpp>

RectElement::RectElement(size_t w, size_t h, u32 color)
{
    this->fb.width = w;
    this->fb.height = h;
    this->color = color;
}
void RectElement::render()
{

    framebuffer_draw_rect(&fb, 0, 0, fb.width, fb.height, color);
}

void View::render()
{
}
void Layout::add_child(View *view)
{
}

LineLayout::LineLayout(bool is_vertical)
{
    this->is_vertical = is_vertical;
    // fb.height = 100;
    // fb.width = 100;

    /* FIXME: this should be done for all classes inheriting layout, but i dont
     * know how */
    list_init_head(&this->children);
}

void LineLayout::render()
{

    size_t p = 0;
    struct list *pos;
    list_for_each(pos, &children)
    {
        View *child = containerof(pos, View, list);
        if (!is_vertical)
        {
            child->x = p;
            child->y = 0;
        }
        else
        {
            child->x = 0;
            child->y = p;
        }

        child->fb.pitch = fb.pitch;
        child->fb.depth = fb.depth;
        child->fb.pixel_width = fb.pixel_width;
        child->fb.buffer =
            (u8 *)fb.buffer + FRAMEBUFFER_OFFSET(fb, child->x, child->y);

        child->render();
        /* FIXME: add offsets here */
        if (!is_vertical)
        {
            p += child->fb.width;
        }
        else
        {
            p += child->fb.height;
        }
    }
}

void LineLayout::add_child(View *child)
{

    if (child->fb.width > fb.width)
    {
        fb.width = child->fb.width;
    }
    if (child->fb.height > fb.height)
    {
        fb.height = child->fb.height;
    }

    list_push(&child->list, &children);
}

/*
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
*/
