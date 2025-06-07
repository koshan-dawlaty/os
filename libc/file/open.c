#include <fcntl.h>
#include <stdio.h>

#if defined(__is_libk)
#include <vfs.h>
#endif

int open(char *path, int flags)
{
#if defined(__is_libk)
    file *f = open_file(path);
    return f->descriptor;
#else
#endif
}
