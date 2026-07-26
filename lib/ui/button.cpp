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

    child->x = 5;
    child->y = 5;

    child->set_fb_parent(fb);

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
