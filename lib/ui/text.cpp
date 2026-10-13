#include <lib/ui/text.hpp>
#include <lib/ui/ui.hpp>

#include <cpp/cpp_c_include_start.hpp>

#include <lib/font.h>
#include <string.h>

#include <cpp/cpp_c_include_end.hpp>

TextElement::TextElement(const char *str, struct PSF_font *font)
    : Element(strlen(str) * font->header.width, font->header.height)
{
    this->str = strdup(str);
    this->font = font;
}

void TextElement::render()
{
    framebuffer_draw_string(&fb, font, str, 0, 0);
}

TextElement::~TextElement()
{
    free(str);
    Element::~Element();
}
