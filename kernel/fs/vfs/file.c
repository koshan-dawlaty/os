#include "assert.h"
#include "dcache.h"
#include "dirent.h"
#include "interrupts/pic.h"
#include "memory/get_free_pages.h"
#include "scheduler.h"
#include "spinlock.h"
#include "vfs_internal.h"
#include <ata/ata.h>
#include <bdev.h>
#include <cdev.h>
#include <device.h>
#include <err.h>
#include <errno.h>
#include <kernel/types.h>
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

file *fdlist_lookup(struct fdlist *list, int fd)
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
    struct fdlist *list = get_current_fdlist();
    if (f != NULL)
    {
        return fdlist_close(list, f->descriptor);
    }
    else
    {
        return -EBADF;
    }
}

file *open_file(const char *path)
{
    dentry *dentry = lookup_path(path, PATHNAME_LOOKUP_FIND);
    if (IS_ERR(dentry))
    {
        return ERR_PTR(dentry);
    }

    file *out = new_file(dentry);

    struct fdlist *list = get_current_fdlist();
    int fd = fdlist_add(list, out);
    out->descriptor = fd;

    return out;
}

file *lookup_descriptor(int fd)
{
    struct fdlist *list = get_current_fdlist();
    return fdlist_lookup(list, fd);
}

int read_dir(struct file *dir, struct dirent *buf, size_t number)
{

    /* TODO: this should maybe be an error code as this is called by a syscall
     */
    NOT_NULL(dir, "directory should not be null");
    NOT_NULL(dir->entry, "directory should not have null dentry");

    ASSERT(dir->entry->type == DENTRY_DIR ||
               dir->entry->type == DENTRY_MOUNT_POINT,
           "the directory entry should represent a directory");

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

int read_file(file *file, char *buf, size_t n, size_t off)
{
    if (file->entry->type == DENTRY_DEVICE)
    {
        struct device *dev = file->entry->dev;
        /* TODO: see if we should use a file offset for block devices */

        if (dev->type == DEV_BLOCK)
        {
            return bdev_read(dev->block, (u8 *)buf, n, off);
        }
        else if (dev->type == DEV_CHARACTER)
        {
            return cdev_read(dev->character, (u8 *)buf, n);
        }
    }
    else
    {
    }

    inode *node = file->entry->inode;

    size_t read_size = n;

    if (off + n > node->size)
    {
        read_size = node->size - off;
    }

    node->sb->type->inode_functions.read_inode(node, (uint8_t *)buf, read_size,
                                               off);

    add_file_offset(file, read_size);

    return read_size;
}

int write_file(file *file, const char *buf, size_t n, size_t off)
{
    if (file->entry->type == DENTRY_DEVICE)
    {
        struct device *dev = file->entry->dev;
        /* TODO: see if we should use a file offset for block devices */

        if (dev->type == DEV_BLOCK)
        {
            return bdev_write(dev->block, (u8 *)buf, n, off);
        }
        else if (dev->type == DEV_CHARACTER)
        {
            return cdev_write(dev->character, (u8 *)buf, n);
        }
    }
    else
    {
    }

    inode *node = file->entry->inode;

    size_t write_size = n;

    node->sb->type->inode_functions.write_inode(node, (const u8 *)buf,
                                                write_size, off);

    add_file_offset(file, write_size);
    return write_size;
}

file *create_file(char *name)
{
    dentry *new = lookup_path(name, PATHNAME_LOOKUP_CREATE);

    if (IS_ERR(new))
    {
        return ERR_PTR(new);
    }

    return new_file(new);
}

void file_free(file *f)
{
    /* TODO: drop dentry */

    free(f);
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

/* TODO: bad name, does not actually add the device to the fdlist */
file *open_device(const char *devname)
{
    dentry *ent = lookup_path(devname, PATHNAME_LOOKUP_FIND);

    if (IS_ERR(ent))
    {
        return (void *)GET_ERR(ent);
    }

    file *out = new_file(ent);
    return out;
}
