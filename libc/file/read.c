#include <stdio.h>
#include <unistd.h>

#if defined(__is_libk)
#include <vfs.h>
#endif

int read(int fd, char *buf, size_t n)
{
#if defined(__is_libk)
    file *f = lookup_descriptor(fd);
    return read_file(f, buf, n, f->offset);
#else
#endif
}
