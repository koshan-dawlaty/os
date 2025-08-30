#ifndef ERROR_H
#define ERROR_H

#include <error.h>

#define MAX_PTR_VAL 4095

/*
 * error code to pointer
 * mainly for negative errors
 */
#define ERR_PTR(err) (void *)err
#define IS_ERR(ptr)                                                            \
    ((unsigned long)(void *)ptr) >= (unsigned long)(-MAX_PTR_VAL)

#define GET_ERR(ptr) -((unsigned long)ptr)

#endif
