#include "fcntl.h"
#include <init.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    init_libc();

    if (argc < 2)
    {
        printf("need more arguments\n");
        exit(1);
    }

    const char *file = argv[1];
    creat(file, 000);
    printf("created:%s\n", file);
    /* int fd = open(file, O_RDONLY); */
    /* printf("opened as:%d\n", fd); */

    exit(0);
}
