#ifndef UI_BUTTON_HPP
#define UI_BUTTON_HPP

#include <lib/ui/ui.hpp>

class ButtonElement : public Element
{
  private:
    struct image img;
    View *child;
    void (*onclick)();

  public:
    ButtonElement(View *child);
    void render() override;
    void handle_element_click() override;
    void set_onclick(void (*onclick)());
};

#endif
