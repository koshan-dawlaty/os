#include <cpp/cpp_c_include_start.hpp>

#include "wm_lib.h"
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <init.h>
#include <lib/array.h>
#include <lib/assert.h>
#include <lib/font.h>
#include <lib/framebuffer.h>
#include <lib/list.h>
#include <lib/log.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <tests/tests.h>
#include <types.h>
#include <unistd.h>
#include <userspace/mouse.h>

#include <cpp/cpp_c_include_end.hpp>

#include <lib/ui/button.hpp>
#include <lib/ui/image.hpp>
#include <lib/ui/text.hpp>
#include <lib/ui/ui.hpp>

struct wm_client client;
struct PSF_font *font;

Layout *root;

Layout *get_dir_layout(DIR *d)
{
    VerticalLineLayout *out = new VerticalLineLayout(0);

    struct dirent *e = readdir(d);
    if (errno != 0)
    {
        return NULL;
    }

    while (e != NULL)
    {
        if (errno != 0)
        {
            return NULL;
        }

        struct stat stat_buf;
        int retval = stat(e->d_name, &stat_buf);

        ButtonElement *button =
            new ButtonElement(new TextElement(e->d_name, font));
        out->add_child(button);

        switch (stat_buf.mode & S_IFMT)
        {
        case S_IFREG:
            button->set_onclick([=](ButtonElement *) -> void
                                { logd("normal file %s", e->d_name); });
            break;
        case S_IFDIR:
            button->set_onclick(
                [=](ButtonElement *) -> void
                {
                    DIR *new_dir = opendir(e->d_name);

                    chdir(e->d_name);
                    auto new_layout = get_dir_layout(new_dir);
                    closedir(new_dir);
                    root->clear_children();
                    framebuffer_draw_rect(&client.fb, 0, 0, client.fb.width,
                                          client.fb.height, 0);
                    root->add_child(new_layout);
                    // ButtonElement *testbutton =
                    //     new ButtonElement(new RectElement(100, 100,
                    //     0xFF00FF));
                    // testbutton->set_onclick([](ButtonElement *) -> void
                    //                         { logd("test button"); });
                    // root->add_child(testbutton);

                    root->render();
                    wm_render(client);
                });
            break;
        default:
            button->set_onclick(
                [=](ButtonElement *) -> void
                { logd("dont know how to open %d", stat_buf.mode & S_IFMT); });
        }

        // out->add_child(new ButtonElement(new RectElement(10, 10, 0xFF)));
        // out->add_child(new RectElement(10, 10, 0xFF));
        // logd("%s ", e->d_name);
        /* printf("%d\n", d->fd); */
        e = readdir(d);

        /* printf("%p\n", malloc(10)); */
    }
    return out;
}

int main()
{
    int font_fd = open("/font.psfu", O_RDONLY);
    font = font_parse_fd(font_fd);
    client = wm_client_connect(600, 600);

    root = new HorizontalLineLayout(10);
    Layout *l = get_dir_layout(opendir("/"));
    root->add_child(l);
    // root->add_child(new ImageElement("dog.tga"));
    root->fb = client.fb;
    root->render();

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
                logd("%d, %d", mouse_x, mouse_y);
                logd("click");
                root->handle_click(mouse_x, mouse_y, 0, 0);
            }

            break;
        }
    }

    exit(0);
}
