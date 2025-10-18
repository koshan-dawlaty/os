#include <fcntl.h>

#ifdef __is_libk
#include <vfs.h>
#endif

int unlink(const char *path)
{
#ifdef __is_libk
    dentry *entry = lookup_path(path);
    return delete_dentry(entry);

#else
    return syscall_wrapper_asm(14, path, 0, 0, 0, 0);
#endif
}
