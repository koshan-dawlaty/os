#include <stdlib.h>
#include <vfs.h>

superblock *new_superblock(size_t blocksize, void *fs_private,
                           const struct fs_type *type)
{

    superblock *out = malloc(sizeof(superblock));

    out->blocksize = blocksize;
    out->fs_private = fs_private;
    out->type = (struct fs_type *)type;

    return out;
}

dentry *new_dentry(char *name, inode *inode)
{
    dentry *out = malloc(sizeof(dentry));
    out->inode = inode;
    out->name = name;

    return out;
}

file *new_file(fd_t descriptor)
{
    file *out = malloc(sizeof(file));
    out->descriptor = descriptor;

    return out;
}

inode *new_inode(ino_t number, void *fs_private)
{
    inode *out = malloc(sizeof(inode));

    out->number = number;
    out->fs_private = fs_private;

    return out;
}

void register_filesystem(struct fs_type *type)
{
}
