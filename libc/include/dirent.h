#ifndef DIRENT_H
#define DIRENT_H

#include <stdio.h>
#include <sys/types.h>

typedef struct
{
    int fd;
    size_t offset;
    /* TODO: put buffer here or something */
} DIR;

struct dirent
{
    ino_t d_ino;
    char d_name[256];
};

int closedir(DIR *);
DIR *opendir(const char *);
struct dirent *readdir(DIR *);
int readdir_r(DIR *, struct dirent *, struct dirent **);
void rewinddir(DIR *);
void seekdir(DIR *, long int);
long int telldir(DIR *);

/*
 * my version of a linux syscall (not standard)
 */
long get_dents(int fd, struct dirent *buf, size_t num);

#endif
