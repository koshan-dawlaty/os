#ifndef CONTAINER_OF_H
#define CONTAINER_OF_H

#ifndef __cplusplus
#define containerof(ptr, type, member)                                         \
    ((type *)((void *)ptr - offsetof(type, member)))
#else
#define containerof(ptr, type, member)                                         \
    ((type *)((u8 *)ptr - offsetof(type, member)))
#endif

#endif
