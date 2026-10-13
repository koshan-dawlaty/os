#ifndef UI_BUTTON_HPP
#define UI_BUTTON_HPP

#include <cpp/cpp_c_include_start.hpp>

#include <lib/log.h>
#include <string.h>

#include <cpp/cpp_c_include_end.hpp>

#include <cpp/function.hpp>
#include <lib/ui/ui.hpp>

class ButtonElement : public Element
{
  private:
    struct image img;

  public:
    View *child;
    function<void(ButtonElement *)> onclick;
    u32 color = 0xFF;
    ButtonElement(View *child);
    ~ButtonElement();
    void render() override;
    void handle_element_click() override;
    void set_onclick(function<void(ButtonElement *)>);
};

#endif
