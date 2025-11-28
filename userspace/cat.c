#include <errno.h>
#include <fcntl.h>
#include <init.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv)
{

    init_libc();

    if (argc <= 1)
    {
        printf("need more arguments\n");
        exit(1);
    }

    int fd = open(argv[1], 0);

    /* printf("FD IS:%d\n", fd); */

    if (fd == -1)
    {
        printf("error:%s\n", strerror(errno));
        exit(1);
    }

    size_t read_size = 10;
    size_t buf_len = read_size;

    char *buf = malloc(buf_len);
    size_t buf_offset = 0;
    size_t bytes_read;
    while ((bytes_read = read(fd, buf + buf_offset, read_size)) >= read_size)
    {
        if (bytes_read < 0)
        {
            printf("invalid fd\n");
        }

        buf_len += read_size;
        buf_offset += read_size;
        buf = realloc(buf, buf_len);
    }
    printf("%s\n", buf);

    close(fd);

    exit(0);
}
