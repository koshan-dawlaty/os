#ifndef ERROR_PTR_H
#define ERROR_PTR_H

#include <types.h>

#define MAX_PTR_VAL 4095

/*
 * error code to pointer
 * mainly for negative errors
 */
#define IS_ERR(ptr)                                                            \
    (((unsigned long)(void *)(ptr_t)ptr) >= (unsigned long)(-MAX_PTR_VAL))

#define GET_ERR(ptr) -((uintptr_t)ptr)

/*
 * creates an error pointer
 * @err: negative error code
 */
static inline void *ERR_PTR(long err)
{
    return (void *)err;
}

#endif
