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

    printf("buf:%s\n", buf);
    while (readsize >= size && (offset / size) < number)
    {
        printf("%d\n", size);
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
