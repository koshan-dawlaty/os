#include "unistd.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    if (argc <= 2)
    {
        printf("need more arguments\n");
        exit(1);
    }
    int fd = open(argv[1], 0);

    write(fd, argv[2], strlen(argv[2]));

    exit(0);
}
