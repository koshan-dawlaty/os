#include <cpp/cpp_c_include_start.hpp>

#include <sys/stat.h>
#include <lib/font.h>
#include <lib/framebuffer.h>
#include <wm_lib.h>

#include <cpp/cpp_c_include_end.hpp>

#include <lib/ui/button.hpp>
#include <lib/ui/image.hpp>
#include <lib/ui/text.hpp>
#include <lib/ui/ui.hpp>

struct PSF_font *font;

struct CanvasState
{
    /* where click starts */
    size_t start_x = 0, start_y = 0;

    size_t last_x = 0, last_y = 0;
    size_t current_x = 0, current_y = 0;
    /* it might be best to have other functions that are called when a click
     * happens, like click_release or something to make this easier and not
     * duplicate */
    bool click_held = false;
    u32 current_color = 0xFF00FF;
};

class PaintTool
{
  public:
    virtual void click_start(struct framebuffer *fb, CanvasState *state)
    {
    }
    virtual void click_held(struct framebuffer *fb, CanvasState *state)
    {
    }
    virtual void click_end(struct framebuffer *fb, CanvasState *state)
    {
    }
};

class PixelDrawTool : public PaintTool
{
    void click_held(struct framebuffer *fb, CanvasState *state) override
    {
        framebuffer_draw_line(fb, state->current_x, state->current_y,
                              state->last_x, state->last_y,
                              state->current_color);
    }
};

class RectDrawTool : public PaintTool
{

    void click_end(struct framebuffer *fb, CanvasState *state) override
    {
		size_t x = MIN(state->current_x, state->start_x);
		size_t y = MIN(state->current_y, state->start_y);
        size_t w = abs(state->current_x - state->start_x);
        size_t h = abs(state->current_y - state->start_y);
        framebuffer_draw_rect(fb, x, y, w, h,
                              state->current_color);
    }
};

class PaintCanvas : public Element
{
  private:
    CanvasState state;
    PaintTool *current_tool = NULL;

  public:
    PaintCanvas(size_t w, size_t h) : Element(w, h)
    {
    }

    void set_color(u32 color)
    {
        state.current_color = color;
    }

    void set_tool(PaintTool *tool)
    {
        current_tool = tool;
    }

    void render() override
    {
    }

    bool handle_mouse_action(size_t click_x, size_t click_y, size_t abs_x,
                             size_t abs_y, struct mouse_event *ev) override
    {
        if (!current_tool)
        {
            return false;
        }

        if (ev->buttons.left_click)
        {
            state.current_x = click_x - x;
            state.current_y = click_y - y;

            if (!state.click_held)
            {
                state.last_x = state.current_x;
                state.last_y = state.current_y;

                current_tool->click_start(&fb, &state);

                state.start_x = state.current_x;
                state.start_y = state.current_y;
            }

            current_tool->click_held(&fb, &state);

            state.last_x = state.current_x;
            state.last_y = state.current_y;

            state.click_held = true;
        }
        else
        {
            if (state.click_held)
            {
                current_tool->click_end(&fb, &state);
                state.click_held = false;
            }
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
            canvas->set_color(color);
        });
    return out;
}

ButtonElement *get_tool_button(PaintCanvas *canvas, PaintTool *tool,
                               const char *image_path)
{
    ButtonElement *out = new ButtonElement(new ImageElement(image_path));
    out->set_onclick([=](ButtonElement *element) -> void
                     { canvas->set_tool(tool); });
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

    Layout *tool_buttons = new HorizontalLineLayout(5);
    tool_buttons->add_child(
        get_tool_button(canvas, new PixelDrawTool(), "images/line.tga"));
    tool_buttons->add_child(
        get_tool_button(canvas, new RectDrawTool(), "images/rectangle.tga"));

    root->add_child(canvas);
    root->add_child(color_buttons);
    root->add_child(tool_buttons);

    root->render();
    root->redraw();

    root->screen->input_loop(root);

    return 0;
}
