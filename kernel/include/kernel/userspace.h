#ifndef USERSPACE_H
#define USERSPACE_H

#include <stdbool.h>


#ifdef CONFIG_x86_64
#define USERADDR_END 0xffffc00000000000
#elif defined(CONFIG_x86_32)
#define USERADDR_END 0xC0000000
#else
#error "unknown architecture"
#endif

static inline bool user_addr_is_valid(void * addr) {
    return addr < USERADDR_END;
}

#endif