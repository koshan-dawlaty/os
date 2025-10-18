#ifndef VFS_H
#define VFS_H
#include "sys/types.h"
#include <stdint.h>
#include <stdio.h>

struct fs_type;

typedef struct
{
    size_t blocksize;
    void *fs_private;
    struct fs_type *type;
} superblock;

typedef struct
{
    // TODO: this should have some way of determining what superblock is parsed
    superblock *(*parse_superblock)();
} superblock_functions;

superblock *new_superblock(size_t blocksize, void *fs_private,
                           const struct fs_type *type);

typedef enum
{
    INODE_FILE,
    INODE_DIR,
    INODE_INVALID,
} inode_type;

typedef size_t inode_size;

typedef struct
{
    ino_t number;
    inode_size size;

    inode_type type;

    void *fs_private;
} inode;

inode *new_inode(ino_t number, inode_size size, inode_type type,
                 void *fs_private);

typedef struct
{
    inode *(*get_inode)(ino_t number, superblock *sb);
    void (*read_inode)(inode *, uint8_t *buf, size_t n, size_t offset,
                       superblock *sb);
    void (*write_inode)(inode *, uint8_t *buf, size_t n, size_t offset,
                        superblock *sb);

    inode *(*alloc_inode)(superblock *sb);

} inode_functions;

typedef struct dentry
{
    char *name;
    struct dentry *parent;
    inode *inode;
} dentry;

/*
 * passing parent as NULL will make the dentry be at the root dir
 */
dentry *new_dentry(char *name, dentry *parent, inode *inode);
void free_dentry(dentry *entry);

typedef struct
{
    // dentry *(*find_dentry)(char *path, superblock *sb);
    dentry **(*get_dentries)(dentry *entry, superblock *sb);
    dentry *(*append_dentry)(dentry *entry, char *name, ino_t number,
                             superblock *sb);

    int (*remove_dentry)(dentry *, char *, superblock *);
} dentry_functions;

typedef uint32_t fd_t;

typedef struct
{
    // where read() / write() will read / write from
    size_t offset;

    fd_t descriptor;
    // this could be inode, as the filename would probably be known. This could
    // also store the name in the file struct itself
    dentry *entry;
} file;

inline void add_file_offset(file *f, size_t add)
{
    f->offset += add;
    if (f->offset > f->entry->inode->size)
    {
        f->offset = f->entry->inode->size;
    }
}

/*
 * initializes descriptor as 0
 */
file *new_file(dentry *entry);
// maybe should return a descriptor
file *open_file(char *path);
int read_file(file *file, char *buf, size_t n, size_t off);
int write_file(file *file, char *buf, size_t n, size_t off);
file *lookup_descriptor(int fd);

typedef struct
{
} file_functions;

struct fs_type
{
    const char *name;

    dentry *(*mount)();

    // bad names
    dentry_functions dentry_functions;
    inode_functions inode_functions;
    superblock_functions superblock_functions;
};

void register_filesystem(struct fs_type *);

void init_vfs();

dentry *lookup_path(const char *path);
file *create_file(char *path);

dentry *get_root_dir();

int delete_dentry(dentry *entry);

#endif
