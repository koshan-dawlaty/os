#include "container_of.h"
#include "err.h"
#include "kernel/utils.h"
#include "lib/list.h"
#include "lib/lru.h"
#include "lib/refcount.h"
#include <panic.h>
#include <stdlib.h>
#include <string.h>
#include <vfs.h>

#define DCACHE_LRU_MAX 10

struct dcache_dir
{
    /* parent directory, NULL is root */
    dentry *dir;
    struct lru_head *values_head;
    struct list *permanent_values;
    list l;
};

struct dcache_value
{
    dentry *entry;
    union
    {
        struct lru_entry lru;
        struct list list;
    };

    refcount *rc;
};

void dcache_free_value(struct dcache_value *value)
{
    free_dentry(value->entry);
}

void dcache_rc_value_release(refcount *rc)
{
    struct dcache_value *value = containerof(rc, struct dcache_value, rc);
    free(value->entry);
}

struct dcache_value *new_dcache_value(dentry *entry)
{
    struct dcache_value *out = malloc(sizeof(struct dcache_value));

    out->entry = entry;
    out->lru = LRU_ENTRY_INIT();
    out->rc = new_refcount();

    return out;
}

struct dcache_dir *new_dcache_dir(dentry *dir)
{
    struct dcache_dir *out = malloc(sizeof(struct dcache_dir));
    out->dir = dir;
    out->values_head = NULL;
    out->permanent_values = NULL;
    out->l = LIST_INIT();

    return out;
}

list dcache_list = LIST_INIT();

struct dcache_dir *get_dcache_dir(dentry *entry)
{

    list *pos;
    list_for_each(pos, &dcache_list)
    {

        struct dcache_dir *dir = containerof(pos, struct dcache_dir, l);

        if (dir->dir == entry)
        {
            return dir;
        }
    }

    return NULL;
}

struct dcache_value *get_dcache_value(char *component, struct dcache_dir *dir)
{

    list2 *pos;
    list2_for_each(pos, &dir->values_head->head->list)
    {
        struct dcache_value *value =
            LRU_LIST_ENTRY(pos, struct dcache_value, lru);

        if (!strcmp(value->entry->name, component))
        {
            /* refcount_get(value->rc); */
            return value;
        }
    }
    return NULL;
}

dentry *get_dcache(char *component, dentry *parent)
{

    struct dcache_dir *dir = get_dcache_dir(parent);

    if (dir == NULL)
    {
        return NULL;
    }

    struct dcache_value *value = get_dcache_value(component, dir);

    if (value == NULL)
    {
        return NULL;
    }

    return value->entry;
}

struct dcache_dir *add_dcache_dir(dentry *dir)
{

    struct dcache_dir *new_dir = new_dcache_dir(dir);
    list_add(&new_dir->l, &dcache_list);

    return new_dir;
}

void add_dcache(dentry *parent, dentry *new_entry)
{

    /* printf("adding %p to cache\n", parent); */

    struct dcache_dir *dir = get_dcache_dir(parent);

    if (dir == NULL)
    {
        dir = add_dcache_dir(parent);
    }

    struct dcache_value *new_value = new_dcache_value(new_entry);

    if (dir->values_head == NULL)
    {

        dir->values_head = new_lru_head(DCACHE_LRU_MAX);
    }
    lru_add(dir->values_head, &new_value->lru);

    struct list2 *pos;
    list2_for_each(pos, &dir->values_head->head->list)
    {
        struct dcache_value *value =
            LRU_LIST_ENTRY(pos, struct dcache_value, lru);
    }
}

void add_dcache_permanent(dentry *parent, dentry *new_entry)
{
    struct dcache_dir *dir = get_dcache_dir(parent);

    if (dir == NULL)
    {
        dir = add_dcache_dir(parent);
    }
    struct dcache_value *new_value = new_dcache_value(new_entry);

    if (dir->permanent_values == NULL)
    {
        new_value->list = LIST_INIT();
        dir->permanent_values = &new_value->list;
    }
    else
    {
        list_add(&new_value->list, dir->permanent_values);
    }
}

void remove_dcache(dentry *entry)
{

    dentry *parent = entry->parent;

    struct dcache_dir *dir = get_dcache_dir(parent);

    if (dir == NULL)
    {
        return;
    }

    struct dcache_value *value = get_dcache_value(entry->name, dir);

    if (value == NULL)
    {
        return;
    }

    /* DOES NOT WORK, this is incorrect!!! */
    lru_remove(&value->lru, dir->values_head);

    dcache_free_value(value);
}

void init_dcache()
{
}
