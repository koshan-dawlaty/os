#include <dirent.h>
#include <init.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{

    init_libc();

    /* printf("%p\n", malloc(4095)); */
    /* printf("%p\n", malloc(4095)); */

    /* exit(0); */

    DIR *d = opendir("/");

    struct dirent *e = readdir(d);
    while (e != NULL)
    {

        printf("%s\n", e->d_name);
        /* printf("%d\n", d->fd); */
        e = readdir(d);

        /* printf("%p\n", malloc(10)); */
    }

    exit(0);
}
