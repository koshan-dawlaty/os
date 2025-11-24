#include "dcache.h"
#include "scheduler.h"
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
#include <vfs.h>

struct fdlist *new_fdlist()
{
    struct fdlist *out = malloc(sizeof(struct fdlist));

    out->len = 0;
    out->files = malloc(0);

    return out;
}

int fdlist_add(struct fdlist *list, file *f)
{
    list->len++;
    list->files = realloc(list->files, list->len * sizeof(file *));
    list->files[list->len - 1] = f;

    return list->len - 1;
}

file *fdlist_lookup(struct fdlist *list, int fd)
{
    if (fd < 0 || (size_t)fd > list->len)
    {
        return NULL;
    }
    return list->files[fd];
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
        /* TODO: add assert task->descriptors != NULL here */
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

file *open_file(char *path)
{
    dentry *dentry = lookup_path(path, PATHNAME_LOOKUP_FIND);
    if (IS_ERR(dentry))
    {
        return (void *)dentry;
    }

    file *out = new_file(dentry);

    struct fdlist *list = get_current_fdlist();
    out->descriptor = fdlist_add(list, out);

    return out;
}

file *lookup_descriptor(int fd)
{
    struct fdlist *list = get_current_fdlist();
    return fdlist_lookup(list, fd);
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

    inode *node = file->entry->inode;

    size_t read_size = n;

    if (off + n > node->size)
    {
        read_size = node->size - off;
    }

    current_superblock->type->inode_functions.read_inode(node, (uint8_t *)buf,
                                                         read_size, off);

    add_file_offset(file, read_size);

    return read_size;
}

int write_file(file *file, char *buf, size_t n, size_t off)
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

    inode *node = file->entry->inode;

    size_t write_size = n;

    current_superblock->type->inode_functions.write_inode(node, (uint8_t *)buf,
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
