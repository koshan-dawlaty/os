#ifndef DIRENT_H
#define DIRENT_H

#include <sys/types.h>

struct dirent
{
    ino_t d_ino;
    char d_name[256];
};

#endif
