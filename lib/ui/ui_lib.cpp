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

Element::Element(size_t w, size_t h) : View()
{
    fb.height = h;
    fb.width = w;
}

RectElement::RectElement(size_t w, size_t h, u32 color)
    : Element(w, h), color(color)
{
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

Layout::Layout()
{
    list_init_head(&children);
}

LineLayout::LineLayout(bool is_vertical, size_t offest)
    : is_vertical(is_vertical), offset(offest)
{
}

void LineLayout::render()
{
    logd("line layout");

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
        if (!is_vertical)
        {
            p += child->fb.width + offset;
        }
        else
        {
            p += child->fb.height + offset;
        }
    }
}

void LineLayout::add_child(View *child)
{

    if (is_vertical)
    {
        if (child->fb.width > fb.width)
        {
            fb.width = child->fb.width;
        }
        fb.height += child->fb.height;
    }
    else
    {
        if (child->fb.height > fb.height)
        {
            fb.height = child->fb.height;
        }
        fb.width += child->fb.width;
    }

    list_push(&child->list, &children);
}

void View::handle_click(size_t, size_t, size_t, size_t)
{
}

void Element::onclick()
{
}

void Element::handle_click(size_t click_x, size_t click_y, size_t abs_x,
                           size_t abs_y)
{
    onclick();
}

void Layout::handle_click(size_t click_x, size_t click_y, size_t abs_x,
                          size_t abs_y)
{

    struct list *pos;
    list_for_each(pos, &children)
    {
        View *child = containerof(pos, View, list);

        size_t x_pos = child->x + abs_x;
        size_t y_pos = child->y + abs_y;

        if (click_x > x_pos && click_x < x_pos + child->fb.width &&
            click_y > y_pos && click_y < y_pos + child->fb.height)
        {

            child->handle_click(click_x, click_y, x_pos, y_pos);
        }
    }
}
