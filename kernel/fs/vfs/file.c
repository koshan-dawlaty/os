#include "assert.h"
#include "dcache.h"
#include "memory/get_free_pages.h"
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
#include <string.h>
#include <vfs.h>

struct fdlist *new_fdlist()
{
    struct fdlist *out = malloc(sizeof(struct fdlist));

    out->len = 0;
    out->files = malloc(0);

    return out;
}

void fdlist_free(struct fdlist *list)
{

    for (size_t i = 0; i < list->len; i++)
    {
        if (list->files[i] != NULL)
        {
            free(list->files[i]);
        }
    }
    free(list->files);
    free(list);
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
    else if (file->entry->type == DENTRY_DIR)
    {
        /* assume buf has space for  n * sizeof(libc_dirent) */
        struct libc_dirent *dir_buf = (struct libc_dirent *)buf;

        dentry **entries = dir_get_dentries(file->entry);
        size_t len = 0;
        while (entries[len] != NULL)
        {
            len++;
        }

        if (off > len)
        {
            return -EINVAL;
        }

        dentry *current = entries[off];

        size_t i = off;
        size_t j = 0;
        while (current != NULL && i < n && i < len)
        {

            /* TODO: not a user pointer */
            char *new_name = strdup(current->name);
            printf("%s\n", new_name);
            printf("%p\n", new_name);
            dir_buf[j].name = new_name;

            /* TODO: this might be invalid if current->type != DENTRY_FILE */
            dir_buf[j].ino = current->inode->number;

            i++;
            j++;
            current = entries[i];
        }

        return j;
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

    inode *node = file->entry->inode;

    size_t write_size = n;

    current_superblock->type->inode_functions.write_inode(node, (const u8 *)buf,
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
