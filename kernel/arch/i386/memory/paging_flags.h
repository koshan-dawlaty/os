#ifndef PAGING_FLAGS_H
#define PAGING_FLAGS_H

// from https://wiki.osdev.org/Paging
#define PAGING_PRESENT 1 << 0
#define PAGING_WRITEABLE 1 << 1
#define PAGING_USER 1 << 2
#define PAGING_WRITE_THROUGH 1 << 3
#define PAGING_WRITE_THROUGH_CACHE 1 << 4

#endif
