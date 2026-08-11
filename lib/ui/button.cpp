#include <cpp/cpp_c_include_start.hpp>

#include <lib/framebuffer.h>
#include <lib/log.h>

#include <cpp/cpp_c_include_end.hpp>

#include <lib/ui/button.hpp>

void ButtonElement::handle_element_click()
{
    onclick(this);
}

void ButtonElement::render()
{
    framebuffer_draw_rect(&fb, 0, 0, fb.width, fb.height, color);
    logd("button screen:%p", screen);

    child->x = 5;
    child->y = 5;

    child->abs_x = x + child->x;
    child->abs_y = y + child->y;

    child->set_fb_parent(fb);
    update_child(child);
    clamp_child_dimensions(child);

    child->render();
}

ButtonElement::ButtonElement(View *child)
    : Element(child->get_width() + 10, child->get_height() + 10), child(child)
{
}

void ButtonElement::set_onclick(function<void(ButtonElement *)> o)
{
    this->onclick = o;
}
