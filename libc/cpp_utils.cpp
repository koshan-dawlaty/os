#include <cpp/cpp_c_include_start.hpp>
#include <lib/assert.h>

#include <lib/log.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <cpp/cpp_c_include_end.hpp>

void *operator new(size_t size)
{
    return malloc(size);
}

void *operator new[](size_t size)
{
    return malloc(size);
}

void operator delete(void *p)
{
    free(p);
}

void operator delete[](void *p)
{
    free(p);
}

void operator delete(void *p, size_t x)
{
    free(p);
}

void operator delete[](void *p, size_t size)
{
    free(p);
}

extern "C" void abort()
{
    loge("c++ maybe called this? (abort())");
    printf("c++ maybe called this? (abort())\n");

    /* it seems like this causes undefined symbol errors, it might be name
     * mangling?, __panic_func() is declared as no return, so that might affect
     * it?  */
    // PANIC("c++ maybe called this? (abort())");
}
