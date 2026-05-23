#include "dcache.h"
#include "lib/refcount.h"
#include "sys/stat.h"
#include "vfs_internal.h"
#include <ata/ata.h>
#include <err_ptr.h>
#include <fcntl.h>
#include <kernel/syscall.h>
#include <lib/assert.h>
#include <lib/log.h>
#include <mutex.h>
#include <scheduler.h>
#include <spinlock.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <task_queue.h>
#include <time.h>
#include <vfs.h>
/* TODO: bad */
#include "../ext2/ext2.h"
#include "../tmpfs/tmpfs.h"
#include "wait_queue.h"

struct fdlist *basic_open_files = NULL;
struct dentry *rootdir = NULL;

struct dentry *get_root_dir()
{
    return rootdir;
}

void init_vfs()
{
    basic_open_files = new_fdlist();

    struct bdev *dev = get_ata_drive(0);

    /* struct fs_type type = tmpfs_type; */
    struct fs_type type = ext2_fs_type;
    rootdir = type.mount(dev);
    rootdir->parent = rootdir;

    /* rootdir->inode->sb->type->dentry_functions.append_dentry( */
    /*     rootdir, "abc", rootdir->inode->sb); */

    /* rootdir = new_dentry(".", NULL, node, DENTRY_DIR); */

    init_dcache();

    vfs_mount(rootdir, strdup("tmpfs"), tmpfs_type, NULL);
    /* create directory for devices */
    vfs_mount(rootdir, strdup("dev"), tmpfs_type, NULL);
    /* mkdir("/dev", 000); */
}

int vfs_mount(struct dentry *root, const char *new_name, struct fs_type fs,
              struct bdev *dev)
{
    struct dentry *new_root = fs.mount(dev);
    new_root->parent = root;
    /* TODO: cast discards qualifiers */
    new_root->name = (char *)new_name;
    add_dcache(root, new_root);

    return 0;
}

struct superblock *new_superblock(size_t blocksize, void *fs_private,
                                  struct bdev *dev, const struct fs_type *type)
{
    struct superblock *out = malloc(sizeof(superblock));

    out->blocksize = blocksize;
    out->fs_private = fs_private;
    out->type = (struct fs_type *)type;
    out->dev = dev;

    return out;
}

struct dentry *new_dentry(char *name, dentry *parent, inode *inode,
                          dentry_type type)
{
    struct dentry *out = malloc(sizeof(dentry));

    out->rc = REFCOUNT_INIT();
    out->inode = inode;
    out->name = name; /* TODO: this should probably be allocated with
                         strdup() but many of the callers of this function
                         already pass a strdup() string to this function */
    out->parent = parent;
    out->type = type;

    return out;
}

struct dentry *dentry_mount_point_alloc(struct superblock *sb,
                                        struct inode *root)
{
    struct dentry *out =
        malloc(sizeof(struct dentry)); /* TODO: separate function to malloc a
                                          struct dentry that everything uses */
    out->rc = REFCOUNT_INIT();
    out->type = DENTRY_MOUNT_POINT;
    out->name = strdup(".");
    out->parent = NULL;
    out->inode = root;
    /* might be redundant */
    out->inode->sb = sb;

    return out;
}

void free_dentry(dentry *entry)
{
    free(entry->name);
    inode_put(entry->inode);
    free(entry);
}

struct file *new_file(struct dentry *entry, int flags)
{
    struct file *out = malloc(sizeof(struct file));
    out->entry = entry;
    out->offset = 0;
    out->flags = flags;

    /* TODO: the entry attribute should be removed and replaced with
     * something using file_private and file operations */
    if (entry != NULL)
    {
        dentry_get(entry);
        out->operations = entry->inode->sb->type->file_operations;
        switch (entry->type)
        {
        case DENTRY_SOCKET:
        case DENTRY_FILE:
            break;
        case DENTRY_MOUNT_POINT:
        case DENTRY_DIR:
            out->dir_context = DIR_CONTEXT_INIT();
            break;
        case DENTRY_INVALID:
            UNREACHABLE("dentry must have valid type");
            break;
        }
    }

    return out;
}

void file_free(struct file *file)
{
    if (file->entry != NULL)
    {
        dentry_put(file->entry);
    }
    /* TODO: files need a refrence count */
    /* free(file); */
}

struct inode *new_inode(ino_t number, inode_size size, void *fs_private,
                        struct superblock *sb)
{
    struct inode *out = malloc(sizeof(struct inode));

    out->rc = REFCOUNT_INIT();
    out->number = number;
    out->size = size;

    out->fs_private = fs_private;

    out->sb = sb;
    mutex_init(&out->lock);

    return out;
}

void inode_free(struct inode *node)
{
    struct fs_type *type = node->sb->type;
    NOT_NULL(type->name, "file system had null name, it is probably invalid")
    NOT_NULL(type->inode_functions.inode_free,
             "filesystem %s did not implement function inode_free", type->name)
    node->sb->type->inode_functions.inode_free(node);
    free(node);
}

int delete_dentry(struct dentry *entry)
{

    int retval = entry->inode->sb->type->dentry_functions.remove_dentry(
        entry->parent, entry->name, entry->inode->sb);
    remove_dcache(entry);

    return retval;
}

void register_filesystem(struct fs_type *type)
{
}

/*
 * TODO: this currently will not use dcache for actual file system entries
 * The issue with this is that both dcache and the filesystem have duplicate
 * information
 */
struct dentry *dir_readdir(struct dentry *entry, struct dir_context *context)
{

    ASSERT(entry->type == DENTRY_DIR || entry->type == DENTRY_MOUNT_POINT,
           "attempt to read directory on non directory dentry");

    struct superblock *sb = entry->inode->sb;
    struct dentry *dcache = dcache_readdir(entry, context);

    /* printf("%p\n", dcache); */

    if (!IS_ERR(dcache) && dcache != NULL)
    {
        /* printf("got %s from dcache\n", dcache->name); */
        context->num_read++;
        return dcache;
    }

    struct dentry *out =
        sb->type->dentry_functions.read_dir(entry, context, sb);

    context->num_read++;
    if (out != NULL)
    {
        /* printf("got %s from not dcache\n", out->name); */
    }

    return out;
}

struct file *fs_poll(struct file **files, size_t num)
{

    struct wait_queue **queues = calloc(num, sizeof(struct wait_queue *));
    struct file *retval = NULL;

    while (retval == NULL)
    {
        struct task *current = get_current_task();
        current->can_sleep = false;
        for (size_t i = 0; i < num; i++)
        {
            bool result = file_poll(files[i], &queues[i]);
            if (result)
            {
                retval = files[i];
                goto end;
            }

            if (queues[i] != NULL)
            {

                task_queue_push(&queues[i]->queue, current);
            }
        }
        current->can_sleep = true;
        /* save_registers(&current->regs); */
        remove_task(current);
    end:
        current->can_sleep = false;
        for (size_t i = 0; i < num; i++)
        {
            if (queues[i] != NULL)
            {
                /* TODO: this function will iterate through all tasks in the
                 * queue as of right now (2026-05-01), which might make this
                 * slow */

                task_queue_remove(&queues[i]->queue, current);
            }
        }

        current->can_sleep = true;
    }
    free(queues);

    return retval;
}

SYSCALL_DEFINE2(SYS_poll, int, int *, fds, size_t, num)
{
    /* FIXME: bounds check on @fds */
    struct file **files = malloc(num * sizeof(struct file));
    for (size_t i = 0; i < num; i++)
    {
        files[i] = lookup_descriptor(fds[i]);
    }

    struct file *file = fs_poll(files, num);

    /* TODO: this loop could be replaced if a struct file knew its
     * descriptor */
    for (size_t i = 0; i < num; i++)
    {
        if (files[i] == file)
        {
            return fds[i];
        }
    }

    return -1;
}
