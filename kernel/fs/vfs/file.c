#include "ata/ata.h"
#include "bdev.h"
#include "cdev.h"
#include "dcache.h"
#include "device.h"
#include "err.h"
#include "kernel/types.h"
#include "vfs_internal.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <vfs.h>

file *open_file(char *path)
{

    dentry *dentry = lookup_path(path, PATHNAME_LOOKUP_FIND);
    if (IS_ERR(dentry))
    {
        return (void *)dentry;
    }

    file *out = new_file(dentry);

    open_files_len++;
    /* TODO: this will result in open_files fragmenting when files are closed */
    open_files = realloc(open_files, (open_files_len) * sizeof(file *));

    /* printf("open_files:%p\n", open_files); */

    open_files[open_files_len - 1] = out;
    out->descriptor = open_files_len - 1;
    /* printf("number:%d, name:%s\n", out->descriptor, out->entry->name); */

    return out;
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

file *lookup_descriptor(int fd)
{
    return open_files[fd];
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
