#include <cpp/cpp_c_include_start.hpp>

#include <lib/assert.h>
#include <lib/img.h>
#include <lib/log.h>

#include <cpp/cpp_c_include_end.hpp>

#include <lib/ui/image.hpp>
#include <lib/ui/ui.hpp>

ImageElement::ImageElement(struct image img, double scale)
    /* FIXME: maybe have a version of image_parse that will load the image into
       memory with a certain scale */
    : Element(img.header.w * scale, img.header.h * scale), img(img),
      scale(scale)
{
}

ImageElement::ImageElement(struct image img) : ImageElement(img, 1)

{
}

ImageElement::ImageElement(const char *pathname, double scale)
    : ImageElement(
          [pathname]()
          {
              struct image img;
              image_parse(pathname, &img);
              return img;
          }(),
          scale)
{
}

ImageElement::ImageElement(const char *pathname) : ImageElement(pathname, 1)
{
}

void ImageElement::render()
{
    framebuffer_draw_img_scale(&fb, 0, 0, scale, img);
}
