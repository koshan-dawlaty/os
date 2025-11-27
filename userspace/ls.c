#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{

    DIR *d = opendir("/dir");
    struct dirent *e = readdir(d);
    while (e != NULL)
    {
        printf("%s\n", e->d_name);
        e = readdir(d);
    }

    exit(0);
}
