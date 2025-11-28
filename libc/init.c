#include <stdio.h>

void init_libc()
{
    /* TODO: maybe make these not writeable */
    stdin = fdopen(0, "");
    stdout = fdopen(1, "");
    stderr = fdopen(2, "");
}
