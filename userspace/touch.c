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

    creat(argv[1], 000);

    exit(0);
}
