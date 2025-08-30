#include <fcntl.h>
#include <sched.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
extern int syscall_wrapper_asm(uint32_t eax, uint32_t ebx, uint32_t ecx,
                               uint32_t edx, uint32_t esi, uint32_t edi);

void main2()
{

    int fd = open("/one", 0);
    char *buf = malloc(10);
    pread(fd, buf, 10, 0);
    printf("%s\n", buf);

    exit(0);

    while (1)
    {
        printf("test\n");
    }
}

int main(int argc, char **argv)
{
    if (argc > 1)
    {
        printf("%d, %s\n", argc, argv[1]);
    }
    main2();
}
