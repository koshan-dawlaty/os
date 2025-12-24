#ifndef VFS_H
#define VFS_H
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

superblock *new_superblock(size_t blocksize, void *fs_private, struct bdev *dev,
                           const struct fs_type *type);

typedef size_t inode_size;

typedef struct inode
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
        inode *inode; /* type == DENTRY_FILE, access with DENTRY_INODE() */
        struct        /* type == DENTRY_MOUNT_POINT */
        {
            struct superblock
                *sb;            /* superblock associated with this filesystem */
            struct inode *root; /* node that contains the directory content of
                                   the root directory for this filesystem,
                                   access with DENTRY_INODE()
                                                */
        } mount_point;
        struct device *dev; /* type == DENTRY_DEVICE  */
    };

    dentry_type type;
} dentry;

/*
 * gets inode from dentry, null if it does not have one
 */
static inline struct inode *DENTRY_INODE(struct dentry *entry)
{
    switch (entry->type)
    {
    case DENTRY_DIR:
        return entry->inode;
    case DENTRY_MOUNT_POINT:
        return entry->mount_point.root;
    default:
        return NULL;
    }
}

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
void free_dentry(dentry *entry);

typedef struct
{
    dentry *(*read_dir)(dentry *entry, struct dir_context *, superblock *sb);
    dentry *(*append_dentry)(dentry *parent, char *name, superblock *sb);

    int (*remove_dentry)(dentry *, char *, superblock *);
} dentry_functions;

typedef uint32_t fd_t;

typedef struct file
{
    /* where read() / write() will read / write from */
    size_t offset;

    union
    {
        struct dir_context dir_context;
    };

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

void fdlist_free(struct fdlist *list);

/* TODO: this name is very similar to the standard readdir() function */
int read_dir(file *dir, struct dirent *buf, size_t number);

/*
 * mounts @fs on @root using @dev
 * @fs is accessible from @name
 */
int vfs_mount(struct dentry *root, const char *new_name, struct fs_type fs,
              struct bdev *dev);

#endif
