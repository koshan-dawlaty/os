#include <cpp/function.hpp>
#include <lib/ui/text_input.hpp>
#include <lib/ui/ui.hpp>

#include <cpp/cpp_c_include_start.hpp>

#include <lib/font.h>
#include <lib/log.h>
#include <string.h>

#include <cpp/cpp_c_include_end.hpp>

TextInputElement::TextInputElement(const char *str, struct PSF_font *font)
    : Element(strlen(str) * font->header.width, font->header.height)
{
    this->str = strdup(str);
    this->font = font;
}

void TextInputElement::render()
{
    framebuffer_draw_string(&fb, font, str, 0, 0);
}

void TextInputElement::set_on_submit(
    function<void(TextInputElement *, char *str)> func)
{
    on_submit = func;
}

void TextInputElement::on_key(char c)
{
    if (c == '\n')
    {
        on_submit(this, str);
    }
    else
    {
        size_t len = strlen(str);
        str = (char *)realloc(str, len + 2);
        str[len + 1] = 0;
        str[len] = c;
        fb.width = (strlen(str) + 1) * font->header.width;
        fb.height = font->header.height;
        render();
        redraw();
    }
}

TextInputElement::~TextInputElement()
{
    free(str);
    Element::~Element();
}
