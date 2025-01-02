#include "kernel/utils.h"

long align_long(long in, long align)
{

    if (in % align == 0)
    {
        return in;
    }

    return (in + align) - (in % align);
}
