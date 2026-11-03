#include <dirent.h>
#include <err_ptr.h>
#include <errno.h>
#include <lib/log.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <types.h>

int main(int argc, char **argv)
{

    /* printf("%p\n", malloc(4095)); */
    /* printf("%p\n", malloc(4095)); */

    /* exit(0); */

    const char *path = ".";
    if (argc >= 2)
    {
        path = argv[1];
    }

    DIR *d = opendir(path);
    if (d == NULL)
    {
		printf("cant open %s\n", path);
		return 1;
    }
    struct dirent *e = readdir(d);
    if (errno != 0)
    {
        printf("ls error:%s\n", strerror(errno));
		return 1;
    }

    while (e != NULL)
    {
        if (errno != 0)
        {
            printf("ls error:%s\n", strerror(errno));
			return 1;
        }

        printf("%s ", e->d_name);
        /* printf("%d\n", d->fd); */
        e = readdir(d);

        /* printf("%p\n", malloc(10)); */
    }
}
