#include "unistd.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{

    if (argc <= 1)
    {
        printf("need more arguments\n");
        exit(1);
    }

    unlink(argv[1]);

    exit(0);
}
