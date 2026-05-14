#include <cstddef>

#include <cpp/cpp_c_include_start.hpp>

#include <stddef.h>
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
