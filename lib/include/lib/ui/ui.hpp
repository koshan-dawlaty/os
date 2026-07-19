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

    virtual void render();
};

class Layout : public View
{
  public:
    struct list children;
    virtual void add_child(View *view);
};
class Element : public View
{
  public:
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

    void render() override;
    void add_child(View *view) override;
    LineLayout(bool is_vertical);
};

#endif
