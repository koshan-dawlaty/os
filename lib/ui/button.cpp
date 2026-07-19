#include <cpp/cpp_c_include_start.hpp>

#include <lib/framebuffer.h>
#include <lib/log.h>

#include <cpp/cpp_c_include_end.hpp>

#include <lib/ui/button.hpp>

void ButtonElement::onclick()
{
    logd("button clicked");
}

void ButtonElement::render()
{
    framebuffer_draw_rect(&fb, 0, 0, fb.width, fb.height, 0xFF);

    child->x = 5;
    child->y = 5;

    child->fb.pitch = fb.pitch;
    child->fb.depth = fb.depth;
    child->fb.pixel_width = fb.pixel_width;
    child->fb.buffer =
        (u8 *)fb.buffer + FRAMEBUFFER_OFFSET(fb, child->x, child->y);

    logd("x:%d, y:%d", child->x, child->y);
    logd("w:%d, h:%d", child->fb.width, child->fb.height);

    child->render();
}

ButtonElement::ButtonElement(View *child)
    : Element(child->fb.width + 10, child->fb.height + 10), child(child)
{
}
