#include <compiler.h>
#include <lib/assert.h>

__noreturn void __stack_chk_fail(void)
{
    PANIC("stack smashing detected");
}
