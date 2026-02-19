#ifndef USERSPACE_H
#define USERSPACE_H

#include <string.h>
#include <stdbool.h>

#ifdef CONFIG_x86_64
#define USERADDR_END 0xffffc00000000000
#elif defined(CONFIG_x86_32)
#define USERADDR_END 0xC0000000
#else
#error "unknown architecture"
#endif

static inline bool user_addr_is_valid(void *addr, size_t len)
{
    return (addr + len) < (void *)USERADDR_END;
}

/* TODO: should return number of bytes that couldnt be copied */
/*
 * returns number of bytes that couldnt be copied, 0 on success
 */
static inline size_t copy_to_user(void * user, const void * kernel, size_t n) {
    if (!user_addr_is_valid(user, n)) {
        return 1;
    }
    memcpy(user, kernel, n);
    return 0;
}


static inline size_t copy_from_user(void * kernel, const void * user, size_t n) {
    if (!user_addr_is_valid(user, n)) {
        return 1;
    }
    memcpy(kernel, user, n);
    return 0;
}

#endif
