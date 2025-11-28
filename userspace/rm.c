#include "unistd.h"
#include <init.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    init_libc();

    if (argc <= 1)
    {
        printf("need more arguments\n");
        exit(1);
    }

    unlink(argv[1]);

    exit(0);
}
