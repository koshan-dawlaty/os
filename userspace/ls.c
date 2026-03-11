#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <init.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{

    init_libc();

    /* printf("%p\n", malloc(4095)); */
    /* printf("%p\n", malloc(4095)); */

    /* exit(0); */

    const char *path = "/";
    if (argc >= 2)
    {
        path = argv[1];
    }

    DIR *d = opendir(path);
    struct dirent *e = readdir(d);
    if (errno != 0)
    {
        printf("ls error:%s\n", strerror(errno));
    }

    while (e != NULL)
    {
        if (errno != 0)
        {
            printf("ls error:%s\n", strerror(errno));
        }

        printf("%s ", e->d_name);
        /* printf("%d\n", d->fd); */
        e = readdir(d);

        /* printf("%p\n", malloc(10)); */
    }
    printf("\n");

    exit(0);
}
