#include "utils.h"
#include <unistd.h>

off_t lseek(int fd, off_t offset, int whence)
{
#ifdef __is_libk
#else
    return syscall_wrapper_asm(15, fd, offset, whence, 0, 0);
#endif
}
