#include <lib/ui/ui.h>

#include "wm_lib.h"
struct wm_client client;
struct PSF_font *font;

int main()
{
    client = wm_client_connect(600, 600);

    int font_fd = open("/font.psfu", O_RDONLY);
    font = font_parse_fd(font_fd);

    struct view *image = ui_image_alloc_str("sokoban/images/box.tga");
    struct view *osdev = ui_image_alloc_str("osdev.tga");

    struct view *blue = ui_rect_alloc(100, 100, 0xFF);
    struct view *purple = ui_rect_alloc(50, 50, 0xFF00FF);
    struct view *green = ui_rect_alloc(50, 50, 0xFF00);
    struct view *cyan = ui_rect_alloc(50, 50, 0xFFFF);
    struct view *button = button_alloc_child(ui_image_alloc_str("dog.tga"));

    struct view *horizontal = layout_line_horizontal_alloc(10);
    struct view *vertical = layout_line_vertical_alloc(10);

    layout_add_child(horizontal, image);
    layout_add_child(horizontal, vertical);
    layout_add_child(horizontal, purple);
    layout_add_child(horizontal, button);

    layout_add_child(vertical, osdev);
    layout_add_child(vertical, green);
    layout_add_child(vertical, blue);

    horizontal->fb = client.fb;

    view_render(horizontal);

    size_t mouse_x = 0, mouse_y = 0;
    while (true)
    {
        wm_render(client);
        struct wm_message message;
        wm_get_message(&client, &message);
        switch (message.header.type)
        {
        case WM_MESSAGE_MOUSE:;
            struct mouse_event *ev = message.message_data;
            mouse_x += ev->x_change;
            mouse_y += ev->y_change;
            if (ev->buttons.left_click)
            {
                view_onclick(horizontal, mouse_x, mouse_y, 0, 0);
            }

            break;
        }
    }
}
