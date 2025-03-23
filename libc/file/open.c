#include <file.h>

#if defined(__is_libk)
#include <vfs.h>
#endif

int open(char *path, int flags)
{
#if defined(__is_libk)
    return open_file(path)->descriptor;
#else
#endif
}
