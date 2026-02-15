#include <err.h>
#include <errno.h>
#include <kernel/utils.h>
#include <container_of.h>
#include <lib/list.h>
#include <lib/lru.h>
#include <lib/refcount.h>
#include <panic.h>
#include <stdlib.h>
#include <string.h>
#include <vfs.h>

#define DCACHE_LRU_MAX 2

struct dcache_dir
{
    /* parent directory, NULL is root */
    struct dentry *dir;
    struct lru_head *values_head;
    struct list *permanent_values;
    list l;
};

struct dcache_value
{
    struct dentry *entry;
    union
    {
        struct lru_entry lru;
        struct list list;
    };

    struct refcount rc;
};

void dcache_free_value(struct dcache_value *value)
{
    free_dentry(value->entry);
}

void dcache_rc_value_release(struct refcount *rc)
{
    struct dcache_value *value = containerof(rc, struct dcache_value, rc);
    dcache_free_value(value);
}

struct dcache_value *new_dcache_value(dentry *entry)
{
    struct dcache_value *out = malloc(sizeof(struct dcache_value));

    out->entry = entry;
    out->lru = LRU_ENTRY_INIT();
    out->rc = REFCOUNT_INIT();

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

    if (dir->values_head != NULL)
    {

        list2 *pos2;
        list2_for_each(pos2, &dir->values_head->head->list)
        {
            struct dcache_value *value =
                LRU_LIST_ENTRY(pos2, struct dcache_value, lru);

            if (!strcmp(value->entry->name, component))
            {
                /* refcount_get(value->rc); */
                return value;
            }
        }
    }

    if (dir->permanent_values != NULL)
    {

        list *pos;
        list_for_each(pos, dir->permanent_values)
        {
            struct dcache_value *value =
                LIST_ENTRY(pos, struct dcache_value, list);

            if (!strcmp(value->entry->name, component))
            {
                return value;
            }
        }
    }

    return NULL;
}

enum dcache_dot_type
{
    DCACHE_DOT_INVALID, /* the component is invalid, probably ".something" */
    DCACHE_DOT_NONE,    /* not "." or "..", but still a valid component */
    DCACHE_DOT,         /* equal to "." */
    DCACHE_DOTDOT,      /* equal to ".." */
};

static inline enum dcache_dot_type dcache_handle_dot(const char *component)
{
    if (component[0] != '.')
    {
        return DCACHE_DOT_NONE;
    }

    /* this won't go past the end of the string (as long as it is null
     * terminated), because the first check ensures that it is at least one byte
     */
    if (component[1] == 0)
    {
        /* component is "." */
        return DCACHE_DOT;
    }
    else if (component[1] != '.' || component[2] != 0)
    {
        return DCACHE_DOT_INVALID;
    }
    /* component is ".." */
    return DCACHE_DOTDOT;
}

struct dentry *get_dcache(char *component, struct dentry *parent)
{

    enum dcache_dot_type type = dcache_handle_dot(component);
    switch (type)
    {
    case DCACHE_DOT:
        return parent;
    case DCACHE_DOTDOT:
        return parent->parent;
    case DCACHE_DOT_INVALID:
        return NULL; /* TODO: this is an error, return an error value */
    case DCACHE_DOT_NONE:
        break;
    }

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

void add_dcache(dentry *parent, dentry *new_entry)
{

    if (new_entry->type == DENTRY_DEVICE ||
        new_entry->type == DENTRY_MOUNT_POINT)
    {
        add_dcache_permanent(parent, new_entry);
        return;
    }

    /* printf("adding %p to cache\n", parent); */

    struct dcache_dir *dir = get_dcache_dir(parent);

    if (new_entry->type == DENTRY_DIR)
    {
        add_dcache_dir(new_entry);
        return;
    }

    if (dir == NULL)
    {
        dir = add_dcache_dir(parent);
    }

    struct dcache_value *new_value = new_dcache_value(new_entry);

    if (dir->values_head == NULL)
    {

        dir->values_head = new_lru_head(DCACHE_LRU_MAX);
    }

    struct lru_entry *removed = lru_add(dir->values_head, &new_value->lru);
    if (removed != NULL)
    {
        struct dcache_value *value =
            containerof(removed, struct dcache_value, lru);

        /* printf("removing %s from dcache\n", value->entry->name); */

        refcount_drop(&value->rc, dcache_rc_value_release);
    }

    struct list2 *pos;
    list2_for_each(pos, &dir->values_head->head->list)
    {
        struct dcache_value *value =
            LRU_LIST_ENTRY(pos, struct dcache_value, lru);
    }
}

/*
 * TODO: this currently does not take entries from temporary dcache, because
 * then it would result in entries not in the dcache not being seen by readdir
 * attempts to read from the directory if it is in the cache
 * if it is not in the cache, returns NULL
 */
struct dentry *dcache_readdir(struct dentry *entry, struct dir_context *context)
{

    /* TODO: this is very stupid, but nothing else should access the dcache
     * offset */

    if (context->dcache_offset ==
        0) /* equivalent to the path being ".", so return the directory that is
              being read from  */
    {
        context->dcache_offset++;
        return entry;
    }
    else if (context->dcache_offset == 1) /* equivalent to the path being "..",
              so return the directory that is being read from's parent  */
    {
        context->dcache_offset++;
        return entry->parent;
    }

    struct dcache_dir *dir = get_dcache_dir(entry);

    if (dir == NULL)
    {
        return NULL;
    }

    /* because of the trick used to get "." and ".." 2 entries have already been
     * read */
    size_t i = 2;
    list *pos;
    list_for_each(pos, dir->permanent_values)
    {
        struct dcache_value *value = LIST_ENTRY(pos, struct dcache_value, list);

        if (i == context->dcache_offset)
        {
            context->dcache_offset++;
            return value->entry;
        }

        i++;
    }

    /* size_t i = 0; */
    /* list2 *pos2; */
    /* list2_for_each(pos2, &dir->values_head->head->list) */
    /* { */
    /*     struct dcache_value *value = */
    /*         LRU_LIST_ENTRY(pos2, struct dcache_value, lru); */

    /*     if (i == context->dcache_offset) */
    /*     { */
    /*         context->dcache_offset++; */
    /*         return value->entry; */
    /*     } */
    /* } */

    if (i == context->dcache_offset + 1)
    {
        /* done reading the directory */
        return NULL;
    }
    else
    {
        return ERR_PTR(-ENOENT);
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
