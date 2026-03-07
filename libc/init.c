#include <stdio.h>
#include <stdlib.h>

void init_libc()
{
    /* TODO: maybe make these not writeable */
    stdin = fdopen(0, "r");
    stdout = fdopen(1, "w");
    stderr = fdopen(2, "w");

    init_heap();
}
