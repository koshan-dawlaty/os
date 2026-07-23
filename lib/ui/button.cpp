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

    child->fb.pitch = fb.pitch;
    child->fb.depth = fb.depth;
    child->fb.pixel_width = fb.pixel_width;
    child->fb.buffer =
        (u8 *)fb.buffer + FRAMEBUFFER_OFFSET(fb, child->x, child->y);

    child->render();
}

ButtonElement::ButtonElement(View *child)
    : Element(child->fb.width + 10, child->fb.height + 10), child(child)
{
}

void ButtonElement::set_onclick(function<void(ButtonElement *)> o)
{
    logd("%p", o);
    logd("%p", onclick);
    this->onclick = o;
}
