#include "dcache.h"
#include "socket.h"
#include "spinlock.h"
#include "sys/stat.h"
#include "unistd.h"
#include "vfs_internal.h"
#include <assert.h>
#include <ata/ata.h>
#include <bdev.h>
#include <cdev.h>
#include <device.h>
#include <dirent.h>
#include <err_ptr.h>
#include <errno.h>
#include <fcntl.h>
#include <interrupts/pic.h>
#include <kernel/types.h>
#include <memory/get_free_pages.h>
#include <mutex.h>
#include <scheduler.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vfs.h>

struct fdlist *new_fdlist()
{
    struct fdlist *out = malloc(sizeof(struct fdlist));

    out->len = 0;
    out->files = malloc(0);
    out->lock = SPINLOCK_INIT();

    return out;
}

void fdlist_free(struct fdlist *list)
{

    for (size_t i = 0; i < list->len; i++)
    {
        if (list->files[i] != NULL)
        {
            file_free(list->files[i]);
        }
    }
    free(list->files);
    free(list);
}

int fdlist_add(struct fdlist *list, file *f)
{
    lock_spinlock(&list->lock);
    list->len++;
    list->files = realloc(list->files, list->len * sizeof(file *));
    list->files[list->len - 1] = f;

    int retval = list->len - 1;
    release_spinlock(&list->lock);
    return retval;
}

void fdlist_set(struct fdlist *list, size_t pos, struct file *f)
{
    spinlock_lock(&list->lock);

    if (pos >= list->len)
    {
        list->len = pos + 1;
        list->files = realloc(list->files, list->len * sizeof(struct file *));
    }
    list->files[pos] = f;
    spinlock_unlock(&list->lock);
}

struct file *fdlist_lookup(struct fdlist *list, int fd)
{
    lock_spinlock(&list->lock);

    if (fd < 0 || (size_t)fd > list->len)
    {
        release_spinlock(&list->lock);
        return NULL;
    }
    void *retval = list->files[fd];
    release_spinlock(&list->lock);
    return retval;
}

int fdlist_close(struct fdlist *list, int fd)
{
    if (fd < 0)
    {
        return -EBADF;
    }

    if ((size_t)fd > list->len)
    {
        return -EBADF;
    }

    file_free(list->files[fd]);
    list->files[fd] = NULL;
    return 0;
}

/* helper function to get the current list of file descriptors */
static inline struct fdlist *get_current_fdlist()
{
    struct task *task = get_current_task();
    if (task == NULL)
    {
        return basic_open_files;
    }
    else
    {
        NOT_NULL(task->descriptors,
                 "task file descriptor list was uninitalized");
        return task->descriptors;
    }
}

int close_file(file *f)
{
    int retval;
    struct fdlist *list = get_current_fdlist();
    if (f != NULL)
    {
        retval = fdlist_close(list, f->descriptor);
    }
    else
    {
        return -EBADF;
    }

    if (f->operations.close != NULL)
    {
        f->operations.close(f);
    }

    return retval;
}

static int read_file_fs(struct file *file, char *buf, size_t n, size_t off)
{
    struct inode *node = file->entry->inode;
    mutex_lock(&node->lock);
    size_t read_size = n;

    if (off + n > node->size)
    {
        read_size = node->size - off;
    }

    inode_read(node, (uint8_t *)buf, read_size, off);

    add_file_offset(file, read_size);
    mutex_unlock(&node->lock);

    return read_size;
}

static int write_file_fs(struct file *file, const char *buf, size_t n,
                         size_t off)
{

    struct inode *node = file->entry->inode;
    mutex_lock(&node->lock);
    size_t write_size = n;
    inode_write(node, (const u8 *)buf, write_size, off);
    mutex_unlock(&node->lock);
    add_file_offset(file, write_size);
    return write_size;
}

const struct file_operations fs_file_operations = {.read = read_file_fs,
                                                   .write = write_file_fs};

extern const struct file_operations device_operations;

/* TODO: the name does not convey that this is always opening a file from the
 * filesystems */
struct file *open_file(const char *path, int flags)
{
    struct dentry *dentry = lookup_path(path, PATHNAME_LOOKUP_FIND);

    if (IS_ERR(dentry))
    {
        return (void *)dentry;
    }

    struct file *out = new_file(dentry, flags);

    switch (dentry->type)
    {
    case DENTRY_DIR:
    case DENTRY_MOUNT_POINT: /* TODO: maybe these have their own operations */
    case DENTRY_FILE:
        out->operations = fs_file_operations;
        break;
    case DENTRY_DEVICE:
        out->operations = device_operations;
        break;
    case DENTRY_SOCKET:
        out->operations = socket_operations;
        out->file_private = dentry->socket;

        break;

    case DENTRY_INVALID:
        UNREACHABLE("invalid dentry type");
    default:
        UNREACHABLE("unknown dentry type / operations");
    }

    struct fdlist *list = get_current_fdlist();
    int fd = fdlist_add(list, out);
    out->descriptor = fd;

    return out;
}

struct file *lookup_descriptor(int fd)
{
    struct fdlist *list = get_current_fdlist();
    return fdlist_lookup(list, fd);
}

int read_dir(struct file *dir, struct dirent *buf, size_t number)
{

    if (dir == NULL)
    {
        return -ENOENT;
    }
    NOT_NULL(dir->entry, "directory should not have null dentry");

    if (dir->entry->type != DENTRY_DIR &&
        dir->entry->type != DENTRY_MOUNT_POINT)
    {
        return -ENOTDIR;
    }

    /* assume buf has space for n * sizeof(dirent) */

    struct dentry *entry = dir_readdir(dir->entry, &dir->dir_context);

    if (entry == NULL)
    {
        return -ENOENT;
    }

    size_t i = 0;

    while (i < number && entry != NULL)
    {
        if (i != 0)
        {
            entry = dir_readdir(dir->entry, &dir->dir_context);
        }

        if (IS_ERR(entry))
        {
            return GET_ERR(entry);
        }

        memcpy(buf[i].d_name, entry->name, strlen(entry->name));

        if (DENTRY_TYPE_HAS_INODE(entry->type))
        {
            buf[i].d_ino = entry->inode->number;
        }

        i++;
    }

    return 0;
}

/* NOT IMPORTANT: maybe inline these */
int read_file(struct file *file, char *buf, size_t n, size_t off)
{

    if (!FLAGS_IS_READABLE(file->flags))
    {
        return -EACCES;
    }

    NOT_NULL(file->operations.read,
             "read called on a file without a read operation");

    return file->operations.read(file, buf, n, off);
}

int write_file(file *file, const char *buf, size_t n, size_t off)
{

    if (!FLAGS_IS_WRITABLE(file->flags))
    {
        return -EACCES;
    }

    NOT_NULL(file->operations.write,
             "write called on a file without a write operation");

    return file->operations.write(file, buf, n, off);
}

struct file *create_file(char *name)
{
    struct dentry *new = lookup_path(name, PATHNAME_LOOKUP_CREATE);

    if (IS_ERR(new))
    {
        return (void *)new;
    }

    /* TODO */
    return new_file(new, O_RDWR);
}

int seek_file(file *f, off_t offset, int whence)
{
    if (f->entry->type != DENTRY_FILE)
    {
        errno = ESPIPE;
        return -1;
    }

    switch (whence)
    {
    case SEEK_CUR:
        f->offset += offset;
        break;
    case SEEK_END:
        f->offset = FILE_SIZE(f) + offset;
        break;

    case SEEK_SET:
        f->offset = offset;
        break;
    }

    return f->offset;
}
