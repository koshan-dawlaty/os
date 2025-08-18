#include "utils.h"
#include <stdint.h>
#include <unistd.h>

int execv(const char *path, char *const argv[])
{
    syscall_wrapper_asm(2, (uint32_t)path, (uint32_t)argv, 0, 0, 0);
    return 0x1234;
}
