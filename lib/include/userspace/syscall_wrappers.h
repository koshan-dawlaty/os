#ifndef LIB_SYSCALL_WRAPPERS_H
#define LIB_SYSCALL_WRAPPERS_H

#include "types.h"
#include <fcntl.h>
#include <lib/font.h>
#include <lib/framebuffer.h>
#include <lib/log.h>
#include <lib/rect.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <userspace/mouse.h>

#ifdef CONFIG_HOST_SDL
static inline long linux_syscall6(long number, long arg1, long arg2, long arg3,
                                  long arg4, long arg5, long arg6)
{
    long ret;

    register long r10 __asm__("r10") = arg4;
    register long r8 __asm__("r8") = arg5;
    register long r9 __asm__("r9") = arg6;

    __asm__ volatile("syscall"
                     : "=a"(ret)
                     : "a"(number), "D"(arg1), "S"(arg2), "d"(arg3), "r"(r10),
                       "r"(r8), "r"(r9)
                     : "rcx", "r11", "memory");

    return ret;
}
static inline int syscall_wrapper_asm(ptr_t ax, ptr_t bx, ptr_t cx, ptr_t dx,
                                      ptr_t si, ptr_t di)
{
    return linux_syscall6(ax, bx, cx, dx, si, di, 0);
}
#else
extern int syscall_wrapper_asm(ptr_t ax, ptr_t bx, ptr_t cx, ptr_t dx, ptr_t si,
                               ptr_t di);
#endif

static inline int send(int socket, void *buf, size_t len)
{
    return write(socket, buf, len);
}

static inline int recv(int socket, void *buf, size_t len)
{
    return read(socket, buf, len);
}

static inline int clone(ptr_t ip)
{
#ifdef CONFIG_HOST_SDL
#else
    return syscall_wrapper_asm(SYS_clone, ip, 0, 0, 0, 0);
#endif
}

static inline int socket_create()
{
#ifdef CONFIG_HOST_SDL
#else
    return syscall_wrapper_asm(SYS_create_socket, 0, 0, 0, 0, 0);
#endif
}

static inline int shared_mem_alloc(void **addr, size_t len)
{
#ifdef CONFIG_HOST_SDL
#else
    return syscall_wrapper_asm(SYS_shared_mem_alloc, (ptr_t)addr, len, 0, 0, 0);
#endif
}

static inline int shared_mem_get(int id, void **p)
{
#ifdef CONFIG_HOST_SDL
#else
    return syscall_wrapper_asm(SYS_shared_mem_get, id, (ptr_t)p, 0, 0, 0);
#endif
}

static inline int shared_mem_resize(int id, size_t len, void **addr)
{
#ifdef CONFIG_HOST_SDL
#else
    return syscall_wrapper_asm(SYS_shared_mem_resize, id, len, (ptr_t)addr, 0,
                               0);
#endif
}

static inline int connect(int socket)
{
#ifdef CONFIG_HOST_SDL
#else
    return syscall_wrapper_asm(SYS_connect, socket, 0, 0, 0, 0);
#endif
}

static inline struct framebuffer *framebuffer_alloc()
{
    struct framebuffer *out = NULL;
#ifdef CONFIG_HOST_SDL
#else
    syscall_wrapper_asm(SYS_get_framebuffer, (ptr_t)&out, 0, 0, 0, 0);
#endif

    return out;
}

static inline int user_mutex_create()
{

#ifdef CONFIG_HOST_SDL
#else
    return syscall_wrapper_asm(SYS_mutex_create, 0, 0, 0, 0, 0);
#endif
}

static inline void user_mutex_lock(int mutex)
{
#ifdef CONFIG_HOST_SDL
#else
    syscall_wrapper_asm(SYS_mutex_lock, mutex, 0, 0, 0, 0);
#endif
}

static inline void user_mutex_unlock(int mutex)
{
#ifdef CONFIG_HOST_SDL
#else
    syscall_wrapper_asm(SYS_mutex_unlock, mutex, 0, 0, 0, 0);
#endif
}

static inline int execv_stdio(const char *path, char *const argv[], int in,
                              int out)
{
#ifdef CONFIG_HOST_SDL
#else
    return syscall_wrapper_asm(SYS_execv_stdio, (ptr_t)path, (ptr_t)argv, in,
                               out, 0);
#endif
}

static inline int pipe(int fds[2])
{
#ifdef CONFIG_HOST_SDL
#else
    return syscall_wrapper_asm(SYS_pipe, (ptr_t)fds, 0, 0, 0, 0);
#endif
}

static inline int poll(int *fds, size_t num)
{
#ifdef CONFIG_HOST_SDL
#else
    /* TODO: error check (maybe) */
    return syscall_wrapper_asm(SYS_poll, (ptr_t)fds, num, 0, 0, 0);
#endif
}

static inline int create_socket_fs(const char *name)
{
#ifdef CONFIG_HOST_SDL
#else
    return syscall_wrapper_asm(SYS_create_socket_fs, (ptr_t)name, 0, 0, 0, 0);
#endif
}

#endif
