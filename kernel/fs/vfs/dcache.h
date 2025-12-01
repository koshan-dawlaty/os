#ifndef DCACHE_H
#define DCACHE_H

#include <vfs.h>

/*
 * gets a dentry from the dcache
 * returns NULL if the dentry was not found
 */
dentry *get_dcache(char *component, dentry *parent);

void add_dcache(dentry *parent, dentry *new_entry);
void init_dcache();
void remove_dcache(dentry *entry);

#endif
