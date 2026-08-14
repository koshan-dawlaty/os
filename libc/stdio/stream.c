#include "lib/log.h"
#include "strings.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

FILE *stdout;
FILE *stdin;
FILE *stderr;

static int string_flags_to_oflags(const char *flags)
{
    if (!strcmp(flags, "r"))
    {
        return O_RDONLY;
    }
    else if (!strcmp(flags, "w"))
    {
        return O_WRONLY;
    }
    else if (!strcmp(flags, "r+"))
    {
        return O_RDWR;
    }
    else if (!strcmp(flags, "c"))
    {
        return O_RDWR | O_CREAT;
    }
    else if (!strcmp(flags, ""))
    {
        /* TODO: this might be an error or undefined */
        return 0;
    }
    else
    {
        printf("unknown stream open flags:%s\n", flags);
        return 0;
    }
}

static FILE *new_stream(int fd, const char *flags, size_t buffer_max)
{
    if (fd < 0)
    {
        return NULL;
    }
    FILE *out = malloc(sizeof(FILE));

    out->fd = fd;

    out->buffer_max = buffer_max;
    out->buffer_size = 0;
    out->buffer = malloc(buffer_max);

    return out;
}

FILE *fdopen(int fd, const char *flags)
{
    return new_stream(fd, flags, 2);
}

FILE *fopen(const char *path, const char *flags)
{

    int oflags = string_flags_to_oflags(flags);
    int fd = open(path, oflags);
    return fdopen(fd, flags);
}

size_t fread(void *restrict buf, size_t size, size_t number, FILE *restrict f)
{
    int fd = f->fd;

    size_t offset = 0;
    size_t readsize = read(fd, buf, size);
    if (readsize != size)
    {
        return readsize / number;
    }
    offset += size;

    while (readsize >= size && (offset / size) < number)
    {

        readsize = read(fd, buf + offset, size);
        if (readsize != size)
        {
            return offset / number;
        }
        offset += readsize;
    }

    return offset;
}

size_t fwrite(const void *restrict buf, size_t size, size_t number, FILE *f)
{
    size_t total_size = size * number;
    size_t write_size = total_size;

    if (write_size > f->buffer_max - f->buffer_size)
    {
        write_size = f->buffer_max - f->buffer_size;
    }

    memcpy(f->buffer + f->buffer_size, buf, write_size);

    if (total_size + f->buffer_size < f->buffer_max)
    {
        f->buffer_size += total_size;
        return total_size;
    }

    f->buffer_size = 0;
    write(f->fd, f->buffer, f->buffer_max);

    size_t bytes_remaining = total_size - write_size;

    if (bytes_remaining != 0)
    {
        write(f->fd, buf + write_size, bytes_remaining);
    }
    memset(f->buffer, 0, f->buffer_max);

    return total_size;
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

int fgetc(FILE *stream)
{
    /* TODO */
    char *buf = malloc(1);
    fread(buf, 1, 1, stream);
    char retval = *buf;
    free(buf);

    return retval;
}
