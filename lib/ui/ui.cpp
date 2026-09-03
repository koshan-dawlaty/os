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

Element::~Element()
{
    View::~View();
}

RectElement::RectElement(size_t w, size_t h, u32 color)
    : Element(w, h), color(color)
{
}

void RectElement::render()
{
    framebuffer_draw_rect(&fb, 0, 0, fb.width, fb.height, color);
}

void View::on_key(char c)
{
}

void View::render()
{
}

void View::update_child(View *child)
{
}

View::~View()
{
    /* it might not be in a list */
    if (list.next != NULL && list.prev != NULL)
    {
        list_remove(&list);
    }
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

void Layout::on_key(char c)
{
    struct list *pos;
    list_for_each(pos, &children)
    {
        View *v = containerof(pos, View, list);
        v->on_key(c);
    }
}

void Layout::clear_children()
{
    // fb.width = 0;
    // fb.height = 0;

    struct list *pos;
    View *prev = NULL;
    list_for_each(pos, &children)
    {
        if (prev != NULL)
        {
            delete prev;
        }
        prev = containerof(pos, View, list);
    }
    if (prev != NULL)
    {
        delete prev;
    }

    list_init_head(&children);
}

Layout::Layout() : View()
{
    fb.width = 0;
    fb.height = 0;
    list_init_head(&children);
}

Layout::~Layout()
{

    View::~View();
    struct list *pos;
    View *prev = NULL;
    list_for_each(pos, &children)
    {
        if (prev != NULL)
        {
            delete prev;
        }
        prev = containerof(pos, View, list);
    }
    if (prev != NULL)
    {
        delete prev;
    }
}

LineLayout::LineLayout(bool is_vertical, size_t offest)
    : is_vertical(is_vertical), offset(offest), Layout()
{
}

LineLayout::~LineLayout()
{
    Layout::~Layout();
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

bool View::handle_mouse_action(size_t, size_t, size_t, size_t,
                               struct mouse_event *ev)
{
    return false;
}

void Element::handle_element_click()
{
}

bool Element::handle_mouse_action(size_t click_x, size_t click_y, size_t abs_x,
                                  size_t abs_y, struct mouse_event *ev)
{
    if (ev->buttons.left_click)
    {
        handle_element_click();
        return true;
    }
    return false;
}

bool Layout::handle_mouse_action(size_t click_x, size_t click_y, size_t abs_x,
                                 size_t abs_y, struct mouse_event *ev)
{
    struct list *pos;

    list_for_each(pos, &children)
    {
        View *child = containerof(pos, View, list);

        size_t x_pos = child->x + abs_x;
        size_t y_pos = child->y + abs_y;

        if (click_x > x_pos && click_x < x_pos + child->get_width() &&
            click_y > y_pos && click_y < y_pos + child->get_height())
        {
            if (child->handle_mouse_action(click_x, click_y, x_pos, y_pos, ev))
            {
                return true;
            }
        }
    }
    return false;
}

size_t View::get_width()
{
    return fb.width;
}
size_t View::get_height()
{
    return fb.height;
}

void ScrollLayout::add_child(View *child)
{
    /* this would push child into this elements children, which we dont want */
    // LineLayout::add_child(child);
    update_child(child);
    array_macro_append(&all_elements, child);

    if (all_elements.num <= max_elements)
    {
        list_push_end(&child->list, &children);
    }
}

void ScrollLayout::clear_children()
{
    LineLayout::clear_children();

    // list_init_head(&children);
    all_elements = {0, 0, 0};
    scroll_offset = max_elements;
}
void ScrollLayout::scroll_up()
{
    if (scroll_offset <= max_elements)
    {
        return;
    }
    scroll_offset--;

    /* remove the item at the top */
    list_pop_end(&children);
    View *new_end = all_elements.items[scroll_offset - max_elements];
    /* add new item at the bottom */
    list_push(&new_end->list, &children);
}

void ScrollLayout::scroll_down()
{
    scroll_offset++;
    // logd("offset:%d, num:%d, max:%d", scroll_offset, all_elements.num,
    //      max_elements);
    if (scroll_offset > all_elements.num - 1)
    {
        scroll_offset = all_elements.num - 1;
        /* dont want to re-add elements that have already been added, so return
         */
        return;
    }

    /* remove the item at the top */
    list_pop(&children);
    View *new_end = all_elements.items[scroll_offset];
    /* add new item at the bottom */
    list_push_end(&new_end->list, &children);
}

bool ScrollLayout::handle_mouse_action(size_t click_x, size_t click_y,
                                       size_t abs_x, size_t abs_y,
                                       struct mouse_event *ev)
{

    if (ev->scroll < 0)
    {
        scroll_down();
    }
    else if (ev->scroll > 0)
    {
        scroll_up();
    }

    bool retval =
        LineLayout::handle_mouse_action(click_x, click_y, abs_x, abs_y, ev);
    framebuffer_draw_rect(&fb, 0, 0, fb.width, fb.height, 0);

    render();
    redraw();

    return retval;
    // struct rect r = {0, 0, (int)screen->get_width(),
    // (int)screen->get_height()}; screen->update_area(r);
}

void ScrollLayout::set_max_elements(size_t new_max)
{
    if (new_max > max_elements)
    {
        for (size_t i = 0; i < new_max - max_elements; i++)
        {
            list_push_end(&all_elements.items[scroll_offset + i]->list,
                          &children);
        }
        scroll_offset += new_max - max_elements;
        render();
        redraw();
    }
    else if (new_max < max_elements)
    {
        for (size_t i = 0; i < max_elements - new_max; i++)
        {
            list_pop_end(&children);
        }
        scroll_offset -= max_elements - new_max;
        render();
        redraw();
    }
    max_elements = new_max;
}
