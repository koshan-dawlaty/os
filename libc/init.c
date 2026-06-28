#include <compiler.h>
#include <lib/assert.h>
#include <stddef.h>
#include <stdio.h>

#include <stdlib.h>

__noreturn void __stack_chk_fail(void)
{
    PANIC("stack smashing detected");
}

void init_libc()
{
    /* TODO: maybe make these not writeable */
    stdin = fdopen(0, "r");
    stdout = fdopen(1, "w");
    stderr = fdopen(2, "w");

    init_heap();
}
