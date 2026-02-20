#ifndef USERSPACE_H
#define USERSPACE_H

#include <err.h>
#include <errno.h>
#include <lib/array.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#ifdef CONFIG_x86_64
#define USERADDR_END 0xffffc00000000000
#elif defined(CONFIG_x86_32)
#define USERADDR_END 0xC0000000
#else
#error "unknown architecture"
#endif

/* TODO: this should check if the addr is writable */
static inline bool user_addr_is_valid(const void *addr, size_t len)
{
    return (addr + len) < (void *)USERADDR_END;
}

/* TODO: should return number of bytes that couldnt be copied */
/*
 * returns number of bytes that couldnt be copied, 0 on success
 */
static inline size_t copy_to_user(void *user, const void *kernel, size_t n)
{
    if (!user_addr_is_valid(user, n))
    {
        return 1;
    }
    memcpy(user, kernel, n);
    return 0;
}

static inline size_t copy_from_user(void *kernel, const void *user, size_t n)
{
    if (!user_addr_is_valid(user, n))
    {
        return 1;
    }
    memcpy(kernel, user, n);
    return 0;
}

static inline int strncpy_from_user(char *kernel, const char *user, size_t n)
{

    /* we need to read at least the null terminator */
    if (!user_addr_is_valid((void *)user, 1))
    {
        return -EFAULT;
    }

    size_t i;
    for (i = 0; i < (n - 1) && user[i] != 0; i++)
    {
        if (!user_addr_is_valid((void *)user + i, 1))
        {
            return -EFAULT;
        }
        kernel[i] = user[i];
    }
    kernel[i] = 0;

    /* TODO: return something else? */
    return n;
}

#define USER_PATH_MAX 1024

/*
 * gets a kernel-usable pathname from a user pathname
 */
static inline char *get_user_pathname(const char *pathname)
{
    char *out = malloc(USER_PATH_MAX);
    int result = strncpy_from_user(out, pathname, USER_PATH_MAX);
    if (result < 1)
    {
        return ERR_PTR(result);
    }

    return out;
}

#endif
