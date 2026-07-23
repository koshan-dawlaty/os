#ifndef VFS_H
#define VFS_H
#include "container_of.h"
#include <device.h>
#include <dirent.h>
#include <kernel/stacktrace.h>
#include <lib/assert.h>
#include <lib/log.h>
#include <lib/refcount.h>
#include <mutex.h>
#include <scheduler.h>
#include <socket.h>
#include <spinlock.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <task_queue.h>
#include <types.h>
#include <wait_queue.h>

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

#define SUPERBLOCK_INIT(_blocksize, _fs_private, _dev, _type)                  \
    ((struct superblock){.blocksize = _blocksize,                              \
                         .fs_private = _fs_private,                            \
                         .type = _type,                                        \
                         .dev = _dev})

typedef size_t inode_size;

typedef struct inode
{
    ino_t number;
    inode_size size;

    struct superblock *sb;
    struct mutex lock;

    void *fs_private;
    struct refcount rc;
} inode;

struct inode *new_inode(ino_t number, inode_size size, void *fs_private,
                        struct superblock *sb);

/*
 * frees @node, including filesystem specific data
 */
void inode_free(struct inode *node);

static inline void inode_get(struct inode *node)
{
    refcount_get(&node->rc);
}

static inline void inode_refcount_drop(struct refcount *rc)
{
    struct inode *inode = containerof(rc, struct inode, rc);
    inode_free(inode);
}

static inline void inode_put(struct inode *node)
{
    refcount_drop(&node->rc, inode_refcount_drop);
}

typedef struct inode_functions
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
} dentry_type;

/*
 * true if entry->inode is accessible
 */
#define DENTRY_TYPE_HAS_INODE(type) type == DENTRY_FILE || type == DENTRY_DIR

/*
 * true if it is a mount point or directory (things that act like a directory
 */
#define DENTRY_TYPE_LIKE_DIR(type)                                             \
    (type == DENTRY_MOUNT_POINT || type == DENTRY_DIR)

typedef struct dentry
{
    char *name;
    struct dentry *parent;

    struct refcount rc; /* dcache could already count refrences, but some things
               (like readdir) don't take dcache_values as refrences */

    union
    {
        inode *inode; /* type == DENTRY_FILE || DENTRY_DIR || DENTRY_MOUNT_POINT
                       * if it is a mount point, the inode is the root of the
                       * mounted filesystem
                       */

        /* struct unix_socket *socket; */
    };

    dentry_type type;
} dentry;
void free_dentry(struct dentry *entry);

static inline void dentry_get(struct dentry *entry)
{
    refcount_get(&entry->rc);
}

static inline void dentry_refcount_drop(struct refcount *rc)
{
    struct dentry *entry = containerof(rc, struct dentry, rc);
    if (!strcmp(entry->name,
                "terminal")) /* it seems like the terminal device dentry has the
                                wrong refcount. this could be because it is both
                                the stdout/err for process by default */
    {
        stacktrace_log();
    }

    free_dentry(entry);
}

static inline void dentry_put(struct dentry *entry)
{
    refcount_drop(&entry->rc, dentry_refcount_drop);
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

dentry *dentry_mount_point_alloc(struct superblock *sb, struct inode *root);

typedef struct
{
    dentry *(*read_dir)(dentry *entry, struct dir_context *, superblock *sb);
    dentry *(*append_dentry)(dentry *parent, char *name, enum dentry_type type,
                             superblock *sb);

    int (*remove_dentry)(dentry *, char *, superblock *);
} dentry_functions;

typedef uint32_t fd_t;

#define FLAGS_IS_READABLE(flags)                                               \
    (((flags & O_RDONLY) != 0) || ((flags & O_RDWR) != 0))

#define FLAGS_IS_WRITABLE(flags)                                               \
    (((flags & O_WRONLY) != 0) || ((flags & O_RDWR) != 0))

/* this probably shouldn't be defined in vfs */
typedef u32 poll_flags;
#define POLL_READ (1 << 0)
#define POLL_WRITE (1 << 1)

struct file;
struct file_operations
{
    int (*read)(struct file *, char *buf, size_t n, size_t off);
    int (*write)(struct file *, const char *buf, size_t n, size_t off);
    /*
     * function to close a file. it can be assumed that the file will not be
     * reopened while this is being called, the file is should be removed from
     * the fdlist before this
     */
    int (*close)(struct file *);

    /*
     * returns true if file is readable
     */
    poll_flags (*poll)(struct file *, struct wait_queue **);
};

#define FILE_OPERATIONS_EMPTY                                                  \
    (struct file_operations)                                                   \
    {                                                                          \
        .read = NULL, .write = NULL, .poll = NULL                              \
    }

typedef struct file
{

    int flags;
    size_t offset;
    /* threads can share files, so they need to be refcounted */
    struct refcount rc;

    union
    {
        struct dir_context dir_context;
    };

    struct dentry *entry;

    struct file_operations operations;
    void *file_private;
} file;

static inline bool file_poll(struct file *f, struct wait_queue **queue)
{
    NOT_NULL(f->operations.poll, "file had null poll() function");
    return f->operations.poll(f, queue);
}

struct file *fs_poll(struct file **files, size_t num);

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
struct file *open_file(
    const char *path, int permissions,
    int *fd); /* FIXME: this could maybe not add the file descriptor to the
                 fdlist, having to take a pointer argument is kinda annoying */
int close_file(file *f);
int read_file(file *file, char *buf, size_t n, size_t off);
int write_file(file *file, const char *buf, size_t n, size_t off);
struct file *lookup_descriptor(int fd);

/*
 * the set of file operations for filesystems to operate on the inode of a file
 */
extern const struct file_operations fs_file_operations;

static inline poll_flags file_default_poll(struct file *f,
                                           struct wait_queue **q)
{
    return POLL_READ | POLL_WRITE;
}

int file_read_default(struct file *file, char *buf, size_t n, size_t off);
int file_write_default(struct file *file, const char *buf, size_t n,
                       size_t off);

int read_file_fs(struct file *file, char *buf, size_t n, size_t off);
int write_file_fs(struct file *file, const char *buf, size_t n, size_t off);
#define FS_FILE_OPERATIONS()                                                   \
    ((struct file_operations){.read = read_file_fs,                            \
                              .write = write_file_fs,                          \
                              .poll = file_default_poll})

struct fs_type
{
    const char *name;
    bool is_permanent;

    dentry *(*mount)(struct bdev *dev);

    // bad names
    dentry_functions dentry_functions;
    inode_functions inode_functions;
    superblock_functions superblock_functions;

    struct file_operations file_operations;
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
        struct inode *node;
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

void file_free(struct file *f);

static inline void file_refcount_drop(struct refcount *c)
{
    file_free(containerof(c, struct file, rc));
}
static inline void file_get(struct file *f)
{
    refcount_get(&f->rc);
    if (f->entry != NULL)
    {
        dentry_get(f->entry); /* we are going to be using this file, so
                                    increment it's dentry */
    }
}
static inline void file_put(struct file *f)
{
    refcount_drop(&f->rc, file_refcount_drop);
}

dentry *get_root_dir();

int delete_dentry(dentry *entry);

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

extern struct fdlist *basic_open_files;

struct fdlist *get_current_fdlist();

static inline struct file *fdlist_get(struct fdlist *list, size_t index)
{
    if (index > list->len)
    {
        return NULL;
    }

    /* the other fdlist code should put NULL values at locations with invalid
     * files in the array, so this still returns NULL if the file is invalid  */
    return list->files[index];
}

/*
 * adds the file to the list
 * returns the file descriptor for the file
 */
int fdlist_add(struct fdlist *list, struct file *f);
struct file *fdlist_lookup(struct fdlist *list, int fd);

/*
 * removes at index @fd in @list
 */
int fdlist_close(struct fdlist *list, int fd);
void fdlist_set(struct fdlist *list, size_t pos, struct file *f);

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

    // logd("inode read sb:%p", node->sb);
    // logd("inode read sb type:%p", node->sb->type);
    // logd("inode read sb funcs:%p", &node->sb->type->inode_functions);
    // logd("inode read ptr:%p", node->sb->type->inode_functions.read_inode);
    if (node->sb->type->inode_functions.read_inode == NULL)
    {
        printf("inode read was null\n");
    }
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

size_t dentry_get_pathname(struct dentry *entry, char *buf, size_t len);

#endif
