#ifndef VFS_H
#define VFS_H
#include "device.h"
#include "kernel/types.h"
#include "sys/types.h"
#include <stdint.h>
#include <stdio.h>

struct fs_type;

typedef struct
{
    size_t blocksize;
    void *fs_private;
    struct fs_type *type;

    struct bdev *dev;
} superblock;

typedef struct
{
    // TODO: this should have some way of determining what superblock is parsed
    superblock *(*parse_superblock)(struct bdev *dev);
} superblock_functions;

superblock *new_superblock(size_t blocksize, void *fs_private, struct bdev *dev,
                           const struct fs_type *type);

typedef size_t inode_size;

typedef struct
{
    ino_t number;
    inode_size size;

    superblock *sb;

    void *fs_private;
} inode;

inode *new_inode(ino_t number, inode_size size, void *fs_private,
                 superblock *sb);

typedef struct
{
    inode *(*get_inode)(ino_t number, superblock *sb);
    void (*read_inode)(inode *, uint8_t *buf, size_t n, size_t offset);
    void (*write_inode)(inode *, const uint8_t *buf, size_t n, size_t offset);

    inode *(*alloc_inode)(superblock *sb);

} inode_functions;

typedef enum
{
    DENTRY_FILE,
    DENTRY_DIR,
    DENTRY_INVALID,

    DENTRY_MOUNT_POINT,
    DENTRY_DEVICE,
} dentry_type;

typedef struct dentry
{
    char *name;
    struct dentry *parent;

    union
    {
        inode *inode;
        struct superblock *sb; /* type == DENTRY_MOUNT_POINT */
        struct device *dev;    /* type == DENTRY_DEVICE  */
    };

    dentry_type type;
} dentry;

/*
 * passing parent as NULL will make the dentry be at the root dir
 */
dentry *new_dentry(char *name, dentry *parent, inode *inode, dentry_type type);
void free_dentry(dentry *entry);

typedef struct
{
    // dentry *(*find_dentry)(char *path, superblock *sb);
    dentry **(*get_dentries)(dentry *entry, superblock *sb);
    dentry *(*append_dentry)(dentry *parent, char *name, superblock *sb);

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

static inline void add_file_offset(file *f, size_t add)
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
file *open_file(const char *path);
int close_file(file *f);
int read_file(file *file, char *buf, size_t n, size_t off);
int write_file(file *file, const char *buf, size_t n, size_t off);
file *lookup_descriptor(int fd);

typedef struct
{
} file_functions;

struct fs_type
{
    const char *name;

    dentry *(*mount)(struct bdev *dev);

    // bad names
    dentry_functions dentry_functions;
    inode_functions inode_functions;
    superblock_functions superblock_functions;
};

static inline dentry **dir_get_dentries(dentry *entry)
{
    superblock *sb = entry->inode->sb;
    return sb->type->dentry_functions.get_dentries(entry, sb);
}

void register_filesystem(struct fs_type *);

void init_vfs();

/* pathname.c */
struct pathname_lookup_flags
{
    u8 create : 1;
};
#define PATHNAME_LOOKUP_CREATE (struct pathname_lookup_flags){.create = 1}
#define PATHNAME_LOOKUP_FIND (struct pathname_lookup_flags){.create = 0}

dentry *lookup_path(const char *path, struct pathname_lookup_flags flags);

file *create_file(char *path);

dentry *get_root_dir();

int delete_dentry(dentry *entry);

dentry *new_dentry_dev(char *name, dentry *parent, struct device *dev);

/*
 * creates a device in the file system
 */
file *create_device(char *name, struct device *dev);

/* TODO: this might not work if entry->type != DENTRY_FILE, however it will
 * probably always be */
#define FILE_SIZE(f) f->entry->inode->size

int seek_file(file *f, off_t offset, int whence);

struct fdlist
{
    file **files;
    size_t len;
};
struct fdlist *new_fdlist();

/*
 * adds the file to the list
 * returns the file descriptor for the file
 */
int fdlist_add(struct fdlist *list, file *f);
file *fdlist_lookup(struct fdlist *list, int fd);

#endif
