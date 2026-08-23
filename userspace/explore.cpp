#include <cpp/cpp_c_include_start.hpp>

#include <lib/list.h>

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <init.h>
#include <lib/array.h>
#include <lib/assert.h>
#include <lib/font.h>
#include <lib/framebuffer.h>
#include <lib/img.h>
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
#include <wm_lib.h>

#include <cpp/cpp_c_include_end.hpp>

#include <lib/ui/button.hpp>
#include <lib/ui/image.hpp>
#include <lib/ui/text.hpp>
#include <lib/ui/ui.hpp>

struct wm_client client;
struct PSF_font *font;

Layout *root;
Layout *entries_layout;
Layout *pathname_layout;

struct image folder_img;
struct image unknown_img;
struct image file_img;

char current_dir[100];

void set_dir(DIR *d);

void change_dir(const char *path)
{
    chdir(path);
    getcwd(current_dir, sizeof(current_dir));
    pathname_layout->clear_children();
    pathname_layout->add_child(new TextElement(current_dir, font));
}

void get_dir_layout(DIR *d)
{

    struct dirent *e = readdir(d);
    if (errno != 0)
    {
        return;
    }

    while (e != NULL)
    {
        if (errno != 0)
        {
            return;
        }
        if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, ".."))
        {
            e = readdir(d);
            continue;
        }

        struct stat stat_buf;
        int retval = stat(e->d_name, &stat_buf);

        View *entry_image;
        switch (stat_buf.mode & S_IFMT)
        {
        case S_IFREG:
            entry_image = new ImageElement(file_img);
            break;
        case S_IFDIR:
            entry_image = new ImageElement(folder_img);
            break;
        default:
            entry_image = new ImageElement(unknown_img);
        };

        Layout *button_content = new HorizontalLineLayout(10);
        button_content->add_child(new TextElement(e->d_name, font));
        button_content->add_child(entry_image);

        ButtonElement *button = new ButtonElement(button_content);
        entries_layout->add_child(button);

        switch (stat_buf.mode & S_IFMT)
        {
        case S_IFREG:
            button->set_onclick(
                [=](ButtonElement *) -> void
                {
                    logd("normal file %s", e->d_name);
                    char *args[] = {"edit", e->d_name, NULL};
                    execv("/bin/edit", args);
                });
            break;
        case S_IFDIR:
            button->set_onclick(
                [=](ButtonElement *) -> void
                {
                    DIR *new_dir = opendir(e->d_name);

                    change_dir(e->d_name);
                    set_dir(new_dir);
                    closedir(new_dir);
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
}

void set_dir(DIR *d)
{

    entries_layout->clear_children();
    get_dir_layout(d);

    /* TODO: this is stupid, but right now it needs to be done,
     * as the new entries could be smaller than the old ones,
     * and the current system wont clear the screen where the
     * removed entries were  */
    framebuffer_draw_rect(&client.fb, 0, 0, client.fb.width, client.fb.height,
                          0);

    // ButtonElement *testbutton =
    //     new ButtonElement(new RectElement(100, 100,
    //     0xFF00FF));
    // testbutton->set_onclick([](ButtonElement *) -> void
    //                         { logd("test button"); });
    // root->add_child(testbutton);

    root->render();
    root->redraw();
    logd("done rendering");
    // wm_render(client);
}

int main()
{

    int font_fd = open("/font.psfu", O_RDONLY);
    font = font_parse_fd(font_fd);
    client = wm_client_connect(600, 600);
    framebuffer_draw_rect(&client.fb, 0, 0, client.fb.width, client.fb.height,
                          0);
    pathname_layout = new HorizontalLineLayout(0);

    root = new VerticalLineLayout(10);
    root->screen = new WMScreen(&client);
    root->fb = client.fb;

    image_parse("/images/folder.tga", &folder_img);
    image_parse("/images/file.tga", &file_img);
    image_parse("/images/question.tga", &unknown_img);

    // const char *start_path = "/";
    const char *start_path = "/";

    change_dir(start_path);
    ButtonElement *go_to_root =
        // new ButtonElement(new TextElement("go to root", font));
        new ButtonElement(new ImageElement("/images/house.tga"));
    go_to_root->set_onclick(
        [](ButtonElement *element) -> void
        {
            change_dir("/");
            DIR *d = opendir("/");

            set_dir(d);
            closedir(d);
        });

    ButtonElement *go_up_dir =
        new ButtonElement(new ImageElement("/images/back_arrow.tga"));
    go_up_dir->set_onclick(
        [](ButtonElement *element) -> void
        {
            change_dir("..");
            DIR *d = opendir(".");

            set_dir(d);
            closedir(d);
        });

    Layout *top_bar = new HorizontalLineLayout(5);
    top_bar->add_child(go_to_root);
    top_bar->add_child(go_up_dir);

    entries_layout = new ScrollLayout(0, 10);
    // entries_layout = new VerticalLineLayout(0);
    // entries_layout = new VerticalLineLayout(0);
    get_dir_layout(opendir(start_path));
    root->add_child(entries_layout);

    root->add_child(top_bar);
    root->add_child(pathname_layout);

    // root->add_child(new ImageElement("dog.tga"));

    root->render();
    // root->redraw();
    wm_render(client);

    while (true)
    {
        struct wm_message message;
        wm_get_message(&client, &message);
        switch (message.header.type)
        {
        case WM_MESSAGE_MOUSE:
        {
            struct mouse_event *ev = (struct mouse_event *)message.message_data;
            size_t mouse_x = ev->x_change, mouse_y = ev->y_change;
            if (ev->buttons.left_click || ev->buttons.right_click ||
                ev->buttons.middle_click || ev->scroll != 0)
            {
                root->handle_mouse_action(mouse_x, mouse_y, 0, 0, ev);
                // framebuffer_draw_rect(&client.fb, 0, 0, client.fb.width,
                //                       client.fb.height, 0);
                /* TODO: we should not do this, instead everything that needs to
                 * cause an update (as of right now scroll layout and any button
                 * lambdas) should do it itself and clear the relevant part of
                 * the screen */
                // root->render();
                // root->redraw();
                // wm_render(client);
            }

            break;
        }
        case WM_MESSAGE_KEY:;

            break;
        case WM_MESSAGE_RESIZE:;
            root->fb = client.fb;
            break;
        }
    }

    exit(0);
}
