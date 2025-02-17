#ifndef VFS_H
#define VFS_H

typedef struct
{

} inode_operations;

typedef struct
{
    const char *name;

    inode_operations *inode;

} fs_type;

#endif
