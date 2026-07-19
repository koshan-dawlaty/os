#include <cpp/cpp_c_include_start.hpp>

#include <lib/assert.h>
#include <lib/img.h>
#include <lib/log.h>

#include <cpp/cpp_c_include_end.hpp>

#include <lib/ui/image.hpp>
#include <lib/ui/ui.hpp>

ImageElement::ImageElement(struct image img)
    : Element(img.header.w, img.header.h)
{
    this->img = img;
}

ImageElement::ImageElement(const char *pathname)
    : ImageElement(
          [pathname]()
          {
              struct image img;
              image_parse(pathname, &img);
              return img;
          }())
{
}

void ImageElement::render()
{
    framebuffer_draw_img(&fb, 0, 0, img);
}
