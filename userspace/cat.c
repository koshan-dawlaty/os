#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    if (argc <= 1)
    {
        printf("need more arguments\n");
        exit(1);
    }

    int fd = open(argv[1], 0);

    size_t read_size = 10;
    size_t buf_len = read_size;

    char *buf = malloc(buf_len);
    while (read(fd, buf + (buf_len - read_size), buf_len) >= read_size)
    {
        buf_len += read_size;
        buf = realloc(buf, buf_len);
    }

    printf("%s\n", buf);

    exit(0);
}
