#include "utils.h"
#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>

#if defined(__is_libk)
#include <vfs.h>
#endif

int pread(int fd, void *buf, size_t n, size_t offset)
{
#if defined(__is_libk)
    file *f = lookup_descriptor(fd);
    return read_file(f, buf, n, offset);
#else
    return syscall_wrapper_asm(SYS_pread, fd, (uintptr_t)buf, n, offset, 0);
#endif
}
