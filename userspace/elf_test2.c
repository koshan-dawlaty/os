#include <init.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
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

int main(int argc, char **argv)
{

    /* for (size_t i = 0; i < 2; i++) */
    /* { */
    /*     sched_yield(); */
    /* } */
    /* /1* printf("sending data\n"); *1/ */

    /* printf("elf_test2\n"); */
    send(0, "abc", 4);
    /* send(0, "abc", 4); */

    for (;;)
    {
    }

    exit(0);
}
