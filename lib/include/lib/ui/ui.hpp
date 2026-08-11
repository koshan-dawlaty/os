#ifndef UI_H
#define UI_H

#include <cpp/cpp_c_include_start.hpp>

#include "wm_lib.h"
#include <lib/framebuffer.h>
#include <lib/img.h>
#include <lib/list.h>
#include <lib/log.h>
#include <lib/rect.h>
#include <stddef.h>
#include <stdlib.h>
#include <types.h>

#include <cpp/cpp_c_include_end.hpp>

class Screen
{
  protected:
    struct framebuffer *fb;

  public:
    inline Screen(struct framebuffer *fb) : fb(fb)
    {
    }
    virtual void update_area(struct rect area) = 0;
    inline size_t get_width()
    {
        return fb->width;
    }
    inline size_t get_height()
    {
        return fb->width;
    }
};

/*
 * this class will not do anything when updating an area, this is intended to be
 * used when you want to render straight to a framebuffer
 */
class DefaultScreen : public Screen
{

  public:
    inline void update_area(struct rect area) override
    {
    }

    inline DefaultScreen(struct framebuffer *fb) : Screen(fb) {};
};

class WMScreen : public Screen
{
  private:
    struct wm_client *client;

  public:
    inline WMScreen(struct wm_client *client)
        : client(client), Screen(&client->fb)
    {
    }
    inline void update_area(struct rect area) override
    {
        wm_render_partial(*client, area);
    };
};

class View
{

  public:
    struct framebuffer fb;
    size_t abs_x = 0, abs_y = 0;
    size_t x, y;

    Screen *screen = nullptr;
    View *parent = nullptr;

    struct list list;

    virtual void handle_click(size_t click_x, size_t click_y, size_t abs_x,
                              size_t abs_y);

    virtual void render() = 0;
    size_t get_width();
    size_t get_height();
    void clamp_child_dimensions(View *child);

    virtual void update_child(View *child);

    inline void redraw()
    {
        // logd("%d, %d %d, %d", x, y, get_width(), get_height());
        screen->update_area(
            {(int)abs_y, (int)abs_y, (int)get_width(), (int)get_height()});
    }

    /*
     * makes this View's framebuffer a child of the parent framebuffer, so it
     * will be rendered using this View's position as an offset into the parent
     * framebuffer, should be called by Layout::render() to set children of the
     * layout
     */
    inline void set_fb_parent(struct framebuffer parent)
    {
        logd("setting with parent fb as %d %d", parent.width, parent.height);

        fb.pitch = parent.pitch;
        fb.depth = parent.depth;
        fb.pixel_width = parent.pixel_width;
        fb.buffer = (u8 *)parent.buffer + FRAMEBUFFER_OFFSET(parent, x, y);
    }
};

class Layout : public View
{
  public:
    struct list children;
    virtual void add_child(View *view);
    virtual void update_child(View *view) override;
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
    void update_child(View *view) override;
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
