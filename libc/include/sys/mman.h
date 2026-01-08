#ifndef SYS_MMAN_H
#define SYS_MMAN_H

#include <kernel_syscall.h>
#include <stdint.h>
#include <sys/types.h>
#include <syscall.h>

void *mmap(void *addr, size_t len, int prot, int flags, off_t off);

#endif
