#include <file.h>
#include <stdio.h>

#if defined(__is_libk)
#include <vfs.h>
#endif

int read(int fd, char *buf, size_t n)
{
#if defined(__is_libk)
    file *f = lookup_descriptor(fd);
    // TODO: offset should not be 0
    return read_file(lookup_descriptor(fd), buf, n, 0);
#else
#endif
}
