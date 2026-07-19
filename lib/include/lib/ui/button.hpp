#ifndef UI_BUTTON_HPP
#define UI_BUTTON_HPP

#include <lib/ui/ui.hpp>

class ButtonElement : public Element
{
  private:
    struct image img;
    View *child;

  public:
    ButtonElement(View *child);
    void render() override;
    void onclick() override;
};

#endif
