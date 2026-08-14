#ifndef UI_IMAGE_HPP
#define UI_IMAGE_HPP

#include <lib/ui/ui.hpp>

class ImageElement : public Element
{
  private:
    struct image img;
    double scale = 1;

  public:
    ImageElement(struct image img, double scale);
    ImageElement(struct image img);
    ImageElement(const char *pathname);
    ImageElement(const char *pathname, double scale);
    void render() override;
};

#endif
