#ifndef UI_TEXT_INPUT_HPP
#define UI_TEXT_INPUT_HPP

#include <cpp/cpp_c_include_start.hpp>

#include <lib/font.h>

#include <cpp/cpp_c_include_end.hpp>

#include <cpp/function.hpp>
#include <lib/ui/ui.hpp>

class TextInputElement : public Element
{
  public:
    TextInputElement(const char *str, struct PSF_font *font);
    ~TextInputElement();
    function<void(TextInputElement *, char *str)> on_submit;

    void on_key(char c) override;
    void render() override;
    void set_on_submit(function<void(TextInputElement *, char *str)> func);

  private:
    char *str;
    struct PSF_font *font;
};

#endif
