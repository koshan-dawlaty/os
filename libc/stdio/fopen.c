#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

FILE *fopen(const char *path, const char *flags)
{
    FILE *out = malloc(sizeof(FILE));
    out->fd = open((char *)path, 0);

    return out;
}

size_t fread(void *restrict buf, size_t size, size_t number, FILE *restrict f)
{
    int fd = f->fd;

    size_t offset = 0;
    size_t readsize = read(fd, buf, size);
    offset += size;

    while (readsize >= size && (offset / size) < number)
    {
        readsize = read(fd, buf + offset, size);
        offset += readsize;
    }

    return offset;
}
