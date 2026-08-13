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

void View::update_child(View *child)
{
}

void View::clamp_child_dimensions(View *child)
{
    if (child->fb.width + child->abs_x > screen->get_width())
    {
        child->fb.width = screen->get_width();
    }
    if (child->fb.height + child->abs_y > screen->get_height())
    {
        child->fb.height = screen->get_height() - child->abs_y;
    }
}

void Layout::update_child(View *child)
{
    child->screen = screen;
    child->parent = this;

    View::update_child(child);

    /* this will be redundant in some cases */
    if (parent != nullptr)
    {
        parent->update_child(this);
    }
}

void Layout::add_child(View *view)
{
    list_push(&view->list, &children);
    update_child(view);
}

void Layout::clear_children()
{
    /* FIXME: free children */
    list_init_head(&children);
}

Layout::Layout() : View()
{
    fb.width = 0;
    fb.height = 0;
    list_init_head(&children);
}

LineLayout::LineLayout(bool is_vertical, size_t offest)
    : is_vertical(is_vertical), offset(offest), Layout()
{
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

        child->abs_x = x + child->x;
        child->abs_y = y + child->y;

        child->set_fb_parent(fb);
        child->screen = screen;

        clamp_child_dimensions(child);

        child->render();
        if (!is_vertical)
        {
            p += child->get_width() + offset;
        }
        else
        {
            p += child->get_height() + offset;
        }
    }
}

void LineLayout::update_child(View *child)
{

    if (is_vertical)
    {
        if (child->get_width() > fb.width)
        {
            fb.width = child->get_width();
        }
        fb.height += child->get_height();
        if (added_first_element)
        {
            fb.height += offset;
        }
    }
    else
    {
        if (child->get_height() > fb.height)
        {
            fb.height = child->get_height();
        }
        fb.width += child->get_width();
        if (added_first_element)
        {
            fb.width += offset;
        }
    }

    added_first_element = true;

    Layout::update_child(child);
}

void View::handle_click(size_t, size_t, size_t, size_t)
{
}

void Element::handle_element_click()
{
}

void Element::handle_click(size_t click_x, size_t click_y, size_t abs_x,
                           size_t abs_y)
{
    handle_element_click();
}

void Layout::handle_click(size_t click_x, size_t click_y, size_t abs_x,
                          size_t abs_y)
{
    struct list *pos;
    list_for_each(pos, &children)
    {
        View *child = containerof(pos, View, list);
        logd("child:%p", child);

        size_t x_pos = child->x + abs_x;
        size_t y_pos = child->y + abs_y;

        if (click_x > x_pos && click_x < x_pos + child->get_width() &&
            click_y > y_pos && click_y < y_pos + child->get_height())
        {
            child->handle_click(click_x, click_y, x_pos, y_pos);
        }
    }
}

size_t View::get_width()
{
    return fb.width;
}
size_t View::get_height()
{
    return fb.height;
}
