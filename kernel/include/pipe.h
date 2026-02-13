#ifndef PIPE_H
#define PIPE_H

#include <vfs.h>

int pipe_open(struct file **read_ptr, struct file **write_ptr);

#endif
