#include <cpp/cpp_c_include_start.hpp>

#include "lib/framebuffer.h"
#include "lib/img.h"
#include "wm_lib.h"
#include <lib/log.h>

#include <cpp/cpp_c_include_end.hpp>

#include <lib/ui/button.hpp>
#include <lib/ui/image.hpp>
#include <lib/ui/text.hpp>
#include <lib/ui/ui.hpp>
#include <stdio.h>
#include <utility>

void test_function(ButtonElement *element)
{
    int i = 10;

    auto lambda = [=](ButtonElement *view) mutable -> void
    {
        logd("osdev button clicked");
        view->color = 0xFF00FF + (i * 1000);
        i++;
        view->render();
    };
    element->set_onclick(lambda);
}

struct wm_client client;
struct PSF_font *font;

int main()
{

    size_t i = 0;
    int x = 10;
    // [x]() -> void { logd("%d", x); }();

    // exit(0);
    client = wm_client_connect(600, 600);
    // struct image img;
    // image_parse("osdev.tga", &img);
    // framebuffer_draw_img(&client.fb, 0, 0, img);
    // wm_render_partial(client, {0, 0, img.header.w, img.header.h});

    // // wm_render(client);
    // wm_render_partial(client, {10, 10, 10, 10});
    // wm_render_partial(client, {100, 20, 10, 20});

    // exit(0);

    int font_fd = open("/font.psfu", O_RDONLY);
    font = font_parse_fd(font_fd);

    // struct view *image = ui_image_alloc_str("sokoban/images/box.tga");
    // struct view *osdev = ui_image_alloc_str("osdev.tga");

    View *blue = new RectElement(100, 100, 0xFF);
    View *purple = new RectElement(50, 50, 0xFF00FF);
    View *green = new RectElement(50, 50, 0xFF00);
    View *cyan = new RectElement(50, 50, 0xFFFF);
    View *red = new RectElement(50, 50, 0xFF0000);

    Layout *horizontal = new HorizontalLineLayout(10);
    Layout *vertical = new VerticalLineLayout(10);

    ButtonElement *button = new ButtonElement(new ImageElement("osdev.tga"));
    test_function(button);
    // auto lambda = [&](ButtonElement *view) -> void
    // {
    //     logd("osdev button clicked");
    //     view->color = 0xFF00FF + (i * 1000);
    //     i++;
    //     view->render();
    // };
    // button->set_onclick(lambda);

    // test_function(button);

    Layout *child = new HorizontalLineLayout(20);
    child->add_child(new RectElement(100, 100, 0xFF0000));
    child->add_child(new ImageElement("dog.tga"));
    child->add_child(new TextElement("dog.tga", font));
    child->add_child(new RectElement(50, 50, 0xFF00FF));

    // horizontal->add_child(blue);
    horizontal->add_child(vertical);
    // horizontal->add_child(child);
    horizontal->add_child(green);

    vertical->add_child(purple);
    // vertical->add_child(new ImageElement("dog.tga"));
    vertical->add_child(child);
    vertical->add_child(button);
    vertical->add_child(cyan);
    // horizontal->add_child(new ButtonElement(child));

    horizontal->fb = client.fb;

    horizontal->render();

    size_t mouse_x = 0, mouse_y = 0;
    while (true)
    {
        wm_render(client);
        struct wm_message message;
        wm_get_message(&client, &message);
        switch (message.header.type)
        {
        case WM_MESSAGE_MOUSE:;
            struct mouse_event *ev = (struct mouse_event *)message.message_data;
            mouse_x += ev->x_change;
            mouse_y += ev->y_change;
            if (ev->buttons.left_click)
            {
                horizontal->handle_click(mouse_x, mouse_y, 0, 0);
            }

            break;
        }
    }
}
