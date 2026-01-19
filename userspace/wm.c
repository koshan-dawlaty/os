#include <fcntl.h>
#include <init.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

/* this is copied from the kernel definition, it might change and break
 * everything and that would be bad */
struct framebuffer
{
    size_t width;
    size_t height;
    size_t pitch;
    size_t depth;
    size_t pixel_width; /* bytes */
    void *buffer;
} __packed;

typedef uintptr_t ptr;
extern int syscall_wrapper_asm(ptr ax, ptr bx, ptr cx, ptr dx, ptr si, ptr di);

struct framebuffer *framebuffer_alloc()
{
    struct framebuffer *out = NULL;
    syscall_wrapper_asm(SYS_get_framebuffer, (ptr)&out, 0, 0, 0, 0);

    return out;
}

int socket_create()
{
    return syscall_wrapper_asm(SYS_create_socket, 0, 0, 0, 0, 0);
}

int send(int socket, void *buf, size_t len)
{
    return syscall_wrapper_asm(SYS_send, socket, (ptr)buf, len, 0, 0);
}

int recv(int socket, void *buf, size_t len)
{
    return syscall_wrapper_asm(SYS_recv, socket, (ptr)buf, len, 0, 0);
}

int clone(ptr ip)
{
    return syscall_wrapper_asm(SYS_clone, ip, 0, 0, 0, 0);
}

void framebuffer_pixel_write(struct framebuffer *f, uint8_t *pixel, int color)
{
    pixel[0] = color & 255;         // BLUE
    pixel[1] = (color >> 8) & 255;  // GREEN
    pixel[2] = (color >> 16) & 255; // RED
}

void framebuffer_draw_rect(struct framebuffer *f, size_t x, size_t y,
                           size_t width, size_t height, int color)
{
    size_t location = y * f->pitch + x * f->pixel_width;

    for (size_t y_pos = y; y_pos < height + y; y_pos++)
    {
        for (size_t x_pos = x; x_pos < width + x; x_pos++)
        {
            uint8_t *pixel = f->buffer + location;
            framebuffer_pixel_write(f, pixel, color);
            location += f->pixel_width;
        }
        location = y_pos * f->pitch + x * f->pixel_width;
    }
}

struct window
{
    size_t x;
    size_t y;
    size_t w;
    size_t h;

    void *buffer;
};

struct window *new_window(size_t x, size_t y, size_t w, size_t h)
{
    struct window *out = malloc(sizeof(struct window));
    out->x = x;
    out->y = y;
    out->w = w;
    out->h = h;

    /* TODO: this assumes 3 bytes per pixel */

    out->buffer = malloc(w * h * 4);

    return out;
}

int shared_mem_alloc(void **addr, size_t len)
{
    return syscall_wrapper_asm(SYS_shared_mem_alloc, (ptr)addr, len, 0, 0, 0);
}

int shared_mem_get(int id, void **p)
{
    return syscall_wrapper_asm(SYS_shared_mem_get, id, (ptr)p, 0, 0, 0);
}

void draw_window(struct framebuffer *b, struct window *win)
{

    for (size_t y = 0; y < win->h; y++)
    {
        /* TODO: assumes 3 bytes per pixel */
        /* printf("%p ", win->buffer + (y * win->w)); */
        memcpy(b->buffer +
                   (((y + win->y) * b->pitch) + (win->x * b->pixel_width)),
               win->buffer + (y * win->w), win->w * b->pixel_width);
    }
}

void thread1()
{
    printf("test\n");

    exit(0);
}

struct window_info
{
    size_t height;
    size_t width;
} __attribute__((packed));

int main()
{
    init_libc();
    printf("window manager\n");

    struct framebuffer *buffer = framebuffer_alloc();

    int socket = socket_create();

    int client_sock = socket_create();

    send(socket, &client_sock, sizeof(int));
    printf("test\n");

    struct window_info win_info;
    recv(client_sock, &win_info, sizeof(struct window_info));
    printf("height:%d\n", win_info.height);

    size_t memory_len = win_info.height * win_info.width * 4;
    void *client_buf;
    int mem_id = shared_mem_alloc(&client_buf, memory_len);
    printf("memory:%d\n", mem_id);

    printf("server buf:%p\n", client_buf);

    send(client_sock, &mem_id, sizeof(int));

    printf("sending code\n");
    int code;
    recv(client_sock, &code, sizeof(int));
    printf("code:%d\n", code);

    printf("%p\n", memory_len);
    /* memcpy(buffer->buffer, client_buf, memory_len); */

    /* printf("%p\n", buffer->pitch); */
    /* framebuffer_draw_rect(buffer, 100, 100, 100, 100, 0xff0000); */

    struct window *w = new_window(400, 600, win_info.width, win_info.height);

    memset(w->buffer, 0, w->h * w->w * 3);

    draw_window(buffer, w);

    exit(0);
}
