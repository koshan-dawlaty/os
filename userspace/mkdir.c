#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

int main(int argc, char *argv[])
{

    if (argc < 2)
    {
        printf("need more arguments\n");
        exit(1);
    }

    const char *dir = argv[1];
    mkdir(dir, 000);
    printf("created:%s\n", dir);
    /* int fd = open(file, O_RDONLY); */

    exit(0);
}
