#ifndef SYS_MMAN_H
#define SYS_MMAN_H

#include <stdint.h>
#include <sys/types.h>

void *mmap(void *addr, size_t len, int prot, int flags, int fildes, off_t off);

#endif
