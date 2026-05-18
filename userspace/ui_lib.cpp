#include <cpp/cpp_c_include_start.hpp>

#include "container_of.h"
#include "lib/framebuffer.h"
#include "lib/log.h"
#include "wm_lib.h"
#include <cstddef>
#include <init.h>
#include <lib/list.h>
#include <lib/rect.h>
#include <stdlib.h>
#include <string.h>

#include <cpp/cpp_c_include_end.hpp>

struct real_point
{
    size_t x;
    size_t y;
};

class Screen
{

  private:
    struct wm_client client;
    struct PSF_font *default_font;

  public:
    Screen(struct wm_client client, struct PSF_font *font)
    {
        default_font = font;
        this->client = client;
    }

    struct PSF_font *get_font()
    {
        return default_font;
    }

    struct framebuffer get_fb()
    {
        return client.fb;
    }

    struct wm_client get_client()
    {
        return client;
    }

    size_t get_width()
    {
        return client.fb.width;
    }

    size_t get_height()
    {
        return client.fb.height;
    }
};

class Element
{

  public:
    struct framebuffer fb;
    double x, y; /* -1 to 1 */

    struct list2c list = LIST2C_INIT();
    struct list2c children;
    Element *parent; /* use a null pointer to represent the top level element */
    Screen *screen;

    struct real_point get_real_pos()
    {
        Element *current = parent;
        Element *prev = NULL;

        struct real_point pos = {0, 0};
        while (current != NULL)
        {
            current = current->parent;
            prev = current;
        }

        return pos;
    }

    Element(double x, double y, size_t width, size_t height, Screen *screen,
            Element *parent)
    {
        this->x = x;
        this->y = y;

        this->screen = screen;
        this->parent = parent;
        this->children = LIST2C_HEAD_INIT(&this->children);

        size_t real_x = 0, real_y = 0;

        struct framebuffer real_fb = screen->get_fb();

        size_t pitch = real_fb.pitch - width;
        fb = FRAMEBUFFER_INIT(width, height, real_fb.pitch, real_fb.depth,
                              real_fb.pixel_width,
                              fb_offset(real_fb, real_x, real_y));
    }

    virtual void draw() = 0;
};

class TextBox : public Element
{

  public:
    char *str;
    TextBox(double x, double y, char *str, Screen *screen, Element *parent)
        : Element(x, y, 100, 100, screen, parent)
    {
        this->str = strdup(str);
    }

    void draw() override
    {
        framebuffer_draw_rect(&fb, 0, 0, 10, 10, 0xFF00FF);
    }
};

int main(int argc, char **argv)
{

    init_libc();

    /* sleep(400); */

    struct wm_client client = wm_client_connect(200, 200);
    int font_fd = open("/font.psfu", O_RDONLY);
    struct PSF_font *font = font_parse(font_fd);

    Screen s = Screen(client, font);
    TextBox t = TextBox(0, 0, "abc", &s, NULL);

    size_t pos = 0;

    t.draw();

    while (1)
    {
        wm_render(client);
        char c = wm_get_key(client);
    }

    exit(1);
}
