#include <cpp/cpp_c_include_start.hpp>

#include "container_of.h"
#include "wm_lib.h"
#include <lib/list.h>
#include <lib/rect.h>
#include <stdlib.h>
#include <string.h>

#include <cpp/cpp_c_include_end.hpp>

class Screen
{
  public:
    struct wm_client client;
    struct PSF_font *default_font;

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
    struct list2c list;
    Screen *screen;

    Element(Screen *screen)
    {
        this->screen = screen;
    }

    virtual void draw() = 0;
};

class TextBox : public Element
{

  public:
    char *str;
    TextBox(char *str, Screen *screen) : Element(screen)
    {
        this->str = strdup(str);
    }

    void draw() override
    {
        // client_draw_string(this->screen->client, this->screen->default_font,
        //                    this->str, );
    }
};

struct ui_element;
struct ui_element_operations
{

    void (*draw)(struct ui_element *element, size_t x, size_t y);
};

struct ui_element
{
    struct rect area;
    struct ui_element_operations operations;
    void *p;

    struct list2c children;

    struct list2c list;
};

struct ui_app
{
};

struct PSF_font *font;

void textbox_draw(struct ui_element *element, size_t x, size_t y)
{
}

struct ui_element_operations textbox_operations = {.draw = textbox_draw};

struct ui_element *ui_element_alloc(struct rect area,
                                    struct ui_element_operations operations)
{

    View *view = new View();
    struct list2c *ptr = &view->list;
    containerof(ptr, View, list);
    struct ui_element *out =
        (struct ui_element *)malloc(sizeof(struct ui_element));
    out->area = area;
    out->operations = operations;
    out->list = LIST2C_INIT();

    return out;
}

void ui_element_add(struct ui_element *parent, struct ui_element *child)
{
    list2c_push(&child->list, &parent->children);
}

int main()
{
    exit(1);
}
