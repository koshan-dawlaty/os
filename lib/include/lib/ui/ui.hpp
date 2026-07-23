#ifndef UI_H
#define UI_H

#include <cpp/cpp_c_include_start.hpp>

#include <lib/framebuffer.h>
#include <lib/img.h>
#include <lib/list.h>
#include <stdlib.h>
#include <types.h>

#include <cpp/cpp_c_include_end.hpp>

class View
{
  public:
    size_t x, y;

    struct framebuffer fb;
    struct list list;

    virtual void handle_click(size_t click_x, size_t click_y, size_t abs_x,
                              size_t abs_y);

    virtual void render() = 0;
    size_t get_width();
    size_t get_height();
};

class Layout : public View
{
  public:
    struct list children;
    virtual void add_child(View *view) = 0;
    Layout();
    void handle_click(size_t click_x, size_t click_y, size_t abs_x,
                      size_t abs_y) override;
    void clear_children();
};

class Element : public View
{
  public:
    Element(size_t w, size_t h);
    void handle_click(size_t click_x, size_t click_y, size_t abs_x,
                      size_t abs_y) override;
    virtual void handle_element_click();
};

class RectElement : public Element
{
  public:
    void render() override;
    RectElement(size_t w, size_t h, u32 color);
    u32 color;
};
class LineLayout : public Layout
{
  public:
    bool is_vertical;
    size_t offset;

    void render() override;
    void add_child(View *view) override;
    LineLayout(bool is_vertical, size_t offset);

  private:
    bool added_first_element = false;
};

class VerticalLineLayout : public LineLayout
{
  public:
    inline VerticalLineLayout(size_t offset) : LineLayout(true, offset)
    {
    }
};

class HorizontalLineLayout : public LineLayout
{
  public:
    inline HorizontalLineLayout(size_t offset) : LineLayout(false, offset)
    {
    }
};

#endif
