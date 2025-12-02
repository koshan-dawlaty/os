#include <errno.h>
#include <string.h>
int errno;

const char *strerror(int errnum)
{
    if (errnum < 0)
    {
        /* TODO: might cause issues */
        errnum = -errnum;
    }

    switch (errnum)
    {
    case ENOENT:
        return "no file or dir";
    case ENOTDIR:
        return "not a dir";
    case EEXIST:
        return "already exists";
    case ENOEXEC:
        return "not executable";
    default:
        return "no error message for this";
    }
}
