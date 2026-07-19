#ifndef UI_IMAGE_HPP
#define UI_IMAGE_HPP

#include <lib/ui/ui.hpp>

class ImageElement : public Element
{
  private:
    struct image img;

  public:
    ImageElement(struct image img);
    ImageElement(const char *pathname);
    void render() override;
};

#endif
