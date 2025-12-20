#ifndef TMPFS_H
#define TMPFS_H

#include <vfs.h>

/*
struct dentry *tmpfs_dentry_append(struct dentry *vfs_parent, char *name,
                                   struct superblock *sb);

struct dentry *tmpfs_read_dir(struct dentry *vfs_dir,
                              struct dir_context *context,
                              struct superblock *sb);

void tmpfs_read_inode(struct inode *vfs, u8 *buf, size_t n, size_t offset);

void tmpfs_write_inode(struct inode *vfs, const u8 *buf, size_t n,
                       size_t offset);

dentry *tmpfs_mount(struct bdev *dev);
*/

extern const struct fs_type tmpfs_type;
#endif
