#include <fcntl.h>
#include <init.h>
#include <sched.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>
typedef uintptr_t ptr;
extern int syscall_wrapper_asm(ptr ax, ptr bx, ptr cx, ptr dx, ptr si, ptr di);

struct framebuffer *framebuffer_alloc()
{
    struct framebuffer *out = NULL;
    syscall_wrapper_asm(18, (ptr)&out, 0, 0, 0, 0);

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

int shared_mem_alloc(void **addr, size_t len)
{
    return syscall_wrapper_asm(SYS_shared_mem_alloc, (ptr)addr, len, 0, 0, 0);
}

int shared_mem_get(int id, void **p)
{
    return syscall_wrapper_asm(SYS_shared_mem_get, id, (ptr)p, 0, 0, 0);
}

struct window_info
{
    size_t height;
    size_t width;
} __attribute__((packed));

int main(int argc, char **argv)
{
    init_libc();
    char *buf = malloc(10);
    int server_sock;
    recv(0, &server_sock, sizeof(int));
    /* printf("%d\n", server_sock); */

    /* printf("test\n"); */
    struct window_info win = {.height = 100, .width = 100};
    send(server_sock, &win, sizeof(struct window_info));
    sched_yield();

    int mem_id;
    recv(server_sock, &mem_id, sizeof(int));
    printf("memory:%d\n", mem_id);

    void *framebuffer;
    shared_mem_get(mem_id, &framebuffer);

    printf("client buffer:%p\n", framebuffer);
    /* memset(framebuffer, 'a', 100 * 100 * 4); */
    memset(framebuffer, 255, 100 * 100);
    /* memset(framebuffer, 'a', 10); */

    printf("sending code\n");
    int done_code = 123;
    send(server_sock, &done_code, sizeof(int));
    printf("done\n");

    exit(0);
}
