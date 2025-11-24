#include <stdio.h>
#include <unistd.h>

int putc(int c, FILE *stream)
{
    /* TODO: use a buffer */
    char buf[1] = {c};
    write(stream->fd, buf, 1);

    return c;
}
