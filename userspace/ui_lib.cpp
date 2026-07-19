#include <cpp/cpp_c_include_start.hpp>

#include "lib/log.h"
#include "wm_lib.h"

#include <cpp/cpp_c_include_end.hpp>
#include <lib/ui/ui.hpp>
#include <stdio.h>

struct wm_client client;
struct PSF_font *font;

int main()
{
    client = wm_client_connect(600, 600);

    int font_fd = open("/font.psfu", O_RDONLY);
    font = font_parse_fd(font_fd);

    // struct view *image = ui_image_alloc_str("sokoban/images/box.tga");
    // struct view *osdev = ui_image_alloc_str("osdev.tga");

    View *blue = new RectElement(100, 100, 0xFF);
    View *purple = new RectElement(50, 50, 0xFF00FF);
    View *green = new RectElement(50, 50, 0xFF00);
    View *cyan = new RectElement(50, 50, 0xFFFF);

    Layout *horizontal = new LineLayout(false);
    Layout *vertical = new LineLayout(true);

    horizontal->add_child(blue);
    horizontal->add_child(vertical);
    horizontal->add_child(green);

    vertical->add_child(purple);
    vertical->add_child(cyan);

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
                // view_onclick(horizontal, mouse_x, mouse_y, 0, 0);
            }

            break;
        }
    }
}
