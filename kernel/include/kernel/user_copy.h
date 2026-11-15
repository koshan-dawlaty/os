#ifndef USERSPACE_H
#define USERSPACE_H

#include <err_ptr.h>
#include <errno.h>
#include <lib/array.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>



/*
 * returns -EFAULT on fail, 0 on success
 */
size_t copy_to_user(void *user, const void *kernel, size_t n);
size_t copy_from_user(void *kernel, const void *user, size_t n);

int strncpy_from_user(char *kernel, const char *user, size_t n);

#define USER_PATH_MAX 1024

/*
 * gets a kernel-usable pathname from a userspace pathname
 */
 char *get_user_pathname(const char *pathname);

#endif
