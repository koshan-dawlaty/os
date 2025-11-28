#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

FILE *stdout;
FILE *stdin;
FILE *stderr;

FILE *fdopen(int fd, const char *flags)
{
    FILE *out = malloc(sizeof(FILE));
    out->fd = fd;

    return out;
}

FILE *fopen(const char *path, const char *flags)
{
    return fdopen(open(path, 0), flags);
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

size_t fwrite(const void *restrict buf, size_t size, size_t number, FILE *f)
{
    int fd = f->fd;

    size_t offset = 0;
    size_t writesize = write(fd, buf, size);
    offset += size;

    while (writesize >= size && (offset / size) < number)
    {
        writesize = write(fd, buf + offset, size);
        offset += writesize;
    }

    return offset;
}

void rewind(FILE *f)
{
    lseek(f->fd, 0, SEEK_SET);
}

int fputc(int c, FILE *stream)
{
    /* TODO */
    char *buf = malloc(1);
    *buf = c;
    fwrite(buf, 1, 1, stream);
    free(buf);

    return c;
}
