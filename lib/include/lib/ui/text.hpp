#ifndef UI_TEXT_HPP
#define UI_TEXT_HPP

#include <cpp/cpp_c_include_start.hpp>

#include <lib/font.h>

#include <cpp/cpp_c_include_end.hpp>

#include <lib/ui/ui.hpp>

class TextElement : public Element
{
  public:
    TextElement(const char *str, struct PSF_font *font);
    ~TextElement();

    void render() override;

  private:
    char *str;
    struct PSF_font *font;
};

#endif
