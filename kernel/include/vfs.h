#ifndef VFS_H
#define VFS_H
#include "kernel/log.h"
#include "mutex.h"
#include "spinlock.h"
#include <device.h>
#include <dirent.h>
#include <kernel/types.h>
#include <scheduler.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>

struct fs_type;

typedef struct superblock
{
    size_t blocksize;
    void *fs_private;
    struct fs_type *type;

    struct bdev *dev;
} superblock;

typedef struct
{
    // TODO: this should have some way of determining what superblock is parsed
    struct superblock *(*parse_superblock)(struct bdev *dev);
} superblock_functions;

struct superblock *new_superblock(size_t blocksize, void *fs_private,
                                  struct bdev *dev, const struct fs_type *type);

typedef size_t inode_size;

typedef struct inode
{
    ino_t number;
    inode_size size;

    struct superblock *sb;
    struct mutex lock;

    void *fs_private;
} inode;

struct inode *new_inode(ino_t number, inode_size size, void *fs_private,
                        struct superblock *sb);

/*
 * frees @node, including filesystem specific data
 */
void inode_free(struct inode *node);

typedef struct
{
    inode *(*get_inode)(ino_t number, superblock *sb);
    void (*read_inode)(inode *, uint8_t *buf, size_t n, size_t offset);
    void (*write_inode)(inode *, const uint8_t *buf, size_t n, size_t offset);

    /* this function should free the filesystem specific data in @node,
     * NOT delete the inode from the filesystem */
    void (*inode_free)(struct inode *node);

} inode_functions;

typedef enum dentry_type
{
    DENTRY_FILE,
    DENTRY_DIR,
    DENTRY_INVALID,

    DENTRY_MOUNT_POINT,
    DENTRY_DEVICE,
} dentry_type;

/*
 * true if entry->inode is accessible
 */
#define DENTRY_TYPE_HAS_INODE(type) type == DENTRY_FILE || type == DENTRY_DIR

typedef struct dentry
{
    char *name;
    struct dentry *parent;

    union
    {
        inode *inode; /* type == DENTRY_FILE || DENTRY_DIR || DENTRY_MOUNT_POINT
                       * if it is a mount point, the inode is the root of the
                       * mounted filesystem
                       */
        struct device *dev; /* type == DENTRY_DEVICE  */
    };

    dentry_type type;
} dentry;

struct dir_context
{
    size_t num_read;
    size_t dcache_offset; /* the offset that dcache uses internally */
    size_t fs_offset;     /* the offset that the file system uses internally */
};
#define DIR_CONTEXT_INIT() (struct dir_context){.num_read = 0, .fs_offset = 0}

/*
 * passing parent as NULL will make the dentry be at the root dir
 */
dentry *new_dentry(char *name, dentry *parent, inode *inode, dentry_type type);

dentry *dentry_mount_point_alloc(struct superblock *sb, struct inode *root);
void free_dentry(dentry *entry);

typedef struct
{
    dentry *(*read_dir)(dentry *entry, struct dir_context *, superblock *sb);
    dentry *(*append_dentry)(dentry *parent, const char *name,
                             enum dentry_type type, superblock *sb);

    int (*remove_dentry)(dentry *, char *, superblock *);
} dentry_functions;

typedef uint32_t fd_t;

#define FLAGS_IS_READABLE(flags)                                               \
    (((flags & O_RDONLY) != 0) || ((flags & O_RDWR) != 0))

#define FLAGS_IS_WRITABLE(flags)                                               \
    (((flags & O_WRONLY) != 0) || ((flags & O_RDWR) != 0))

struct file;
struct file_operations
{
    int (*read)(struct file *, char *buf, size_t n, size_t off);
    int (*write)(struct file *, const char *buf, size_t n, size_t off);
};

#define FILE_OPERATIONS_EMPTY                                                  \
    (struct file_operations)                                                   \
    {                                                                          \
        .read = NULL, .write = NULL                                            \
    }

typedef struct file
{

    int flags;
    size_t offset;

    union
    {
        struct dir_context dir_context;
    };

    fd_t descriptor;
    struct dentry *entry;

    struct file_operations operations;
    void *file_private;
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
struct file *new_file(struct dentry *entry, int permissions);
struct file *open_file(const char *path, int permissions);
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

dentry *dir_readdir(dentry *entry, struct dir_context *context);

void register_filesystem(struct fs_type *);

void init_vfs();

/* pathname.c */
struct pathname_lookup_flags
{
    u8 create : 1;
    dentry_type type;
    union
    {
        struct device *dev;
    };
};
#define PATHNAME_LOOKUP_CREATE                                                 \
    (struct pathname_lookup_flags)                                             \
    {                                                                          \
        .create = 1, .type = DENTRY_FILE                                       \
    }
#define PATHNAME_LOOKUP_CREATE_DEV(device)                                     \
    (struct pathname_lookup_flags)                                             \
    {                                                                          \
        .create = 1, .type = DENTRY_DEVICE, .dev = device                      \
    }
#define PATHNAME_LOOKUP_FIND (struct pathname_lookup_flags){.create = 0}

dentry *lookup_path(const char *path, struct pathname_lookup_flags flags);

file *create_file(char *path);
void file_free(file *f);

dentry *get_root_dir();

int delete_dentry(dentry *entry);

dentry *new_dentry_dev(char *name, dentry *parent, struct device *dev);

/*
 * creates a device in the file system
 */
file *create_device(char *name, struct device *dev);
file *open_device(const char *name);

/* TODO: this might not work if entry->type != DENTRY_FILE, however it will
 * probably always be */
#define FILE_SIZE(f) f->entry->inode->size

int seek_file(file *f, off_t offset, int whence);

struct fdlist
{
    struct file **files;
    struct spinlock lock;
    size_t len;
};
struct fdlist *new_fdlist();

/*
 * adds the file to the list
 * returns the file descriptor for the file
 */
int fdlist_add(struct fdlist *list, struct file *f);
struct file *fdlist_lookup(struct fdlist *list, int fd);

void fdlist_free(struct fdlist *list);

/* TODO: this name is very similar to the standard readdir() function */
int read_dir(file *dir, struct dirent *buf, size_t number);

/*
 * mounts @fs on @root using @dev
 * @fs is accessible from @name
 */
int vfs_mount(struct dentry *root, const char *new_name, struct fs_type fs,
              struct bdev *dev);

/* TODO: these should return bytes read or something in the future*/
static inline int inode_read(struct inode *node, u8 *buf, size_t n, size_t off)
{

    if (node->sb->type->inode_functions.read_inode == NULL)
    {
        printf("inode read was null\n");
    }
    // logd("inode read sb:%p", node->sb);
    // logd("inode read ptr:%p", node->sb->type->inode_functions.read_inode);
    node->sb->type->inode_functions.read_inode(node, buf, n, off);

    return 0;
}
/* TODO: these should return bytes read or something in the future*/
static inline int inode_write(struct inode *node, const u8 *buf, size_t n,
                              size_t off)
{
    node->sb->type->inode_functions.write_inode(node, buf, n, off);

    return 0;
}

#endif
