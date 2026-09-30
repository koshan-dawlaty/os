#include <cpp/cpp_c_include_start.hpp>

#include <lib/font.h>
#include <lib/framebuffer.h>
#include <wm_lib.h>

#include <cpp/cpp_c_include_end.hpp>

#include <lib/ui/button.hpp>
#include <lib/ui/image.hpp>
#include <lib/ui/text.hpp>
#include <lib/ui/ui.hpp>

struct PSF_font *font;

class PaintCanvas : public Element
{

  public:
    u32 current_color = 0xFF00FF;
    size_t last_x = 0, last_y = 0;
    /* it might be best to have other functions that are called when a click
     * happens, like click_release or something to make this easier and not
     * duplicate */
    bool click_held = false;

    PaintCanvas(size_t w, size_t h) : Element(w, h)
    {
    }

    void render() override
    {
    }

    bool handle_mouse_action(size_t click_x, size_t click_y, size_t abs_x,
                             size_t abs_y, struct mouse_event *ev) override
    {

        if (ev->buttons.left_click)
        {
            int x_pos = click_x - x;
            int y_pos = click_y - y;

            if (!click_held)
            {
                last_x = x_pos;
                last_y = y_pos;
            }

            framebuffer_draw_line(&fb, x_pos, y_pos, last_x, last_y,
                                  current_color);
            last_x = x_pos;
            last_y = y_pos;

            click_held = true;
        }
        else
        {
            click_held = false;
        }
        /* FIXME: do a partial redraw */
        redraw();

        return true;
    }
};

ButtonElement *get_color_button(PaintCanvas *canvas, u32 color)
{
    ButtonElement *out = new ButtonElement(new RectElement(30, 30, color));
    out->set_onclick(
        [=](ButtonElement *element) -> void
        {
            logd("setting color:%p", color);
            canvas->current_color = color;
        });
    return out;
}

int main()
{
    int font_fd = open("/font.psfu", O_RDONLY);
    font = font_parse_fd(font_fd);

    struct wm_client client = wm_client_connect(500, 500);
    framebuffer_draw_rect(&client.fb, 0, 0, client.fb.width, client.fb.height,
                          0);

    Layout *root = new VerticalLineLayout(10);
    root->screen = new WMScreen(&client);
    root->fb = client.fb;

    PaintCanvas *canvas = new PaintCanvas(200, 200);

    Layout *color_buttons = new HorizontalLineLayout(5);
    color_buttons->add_child(get_color_button(canvas, 0x00FF00));
    color_buttons->add_child(get_color_button(canvas, 0xFF0000));
    color_buttons->add_child(get_color_button(canvas, 0x0000FF));
    color_buttons->add_child(get_color_button(canvas, 0xFF00FF));

    root->add_child(canvas);
    root->add_child(color_buttons);

    root->render();
    root->redraw();

    root->screen->input_loop(root);

    return 0;
}
