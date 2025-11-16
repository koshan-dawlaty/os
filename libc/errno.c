#include <errno.h>
#include <string.h>
int errno;

const char *strerror(int errnum)
{
    switch (errnum)
    {
    case ENOENT:
        return "no file or dir";
    case ENOTDIR:
        return "not a dir";
    case EEXIST:
        return "already exists";
    default:
        return "no error message for this";
    }
}
