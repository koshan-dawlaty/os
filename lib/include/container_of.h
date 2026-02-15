#ifndef CONTAINER_OF_H
#define CONTAINER_OF_H

#define containerof(ptr, type, member)                                         \
    ((type *)((void *)ptr - offsetof(type, member)))

#endif
