#include "utils.h"
#include <stdio.h>
#include <unistd.h>

#ifdef __is_libk
#include <kernel/keyboard.h>
#endif

char getchar()
{
#if defined(__is_libk)

    /* printf("test\n"); */
    /* printf("%d\n", key_available); */
    /* while (!is_key_available()) */
    /* { */
    /*     /1* printf("test\n"); *1/ */
    /*     asm volatile("hlt"); */
    /*     /1* sched_yield(); *1/ */
    /* } */
    /* /1* printf("test\n"); *1/ */

    /* key_available = 0; */
    char c = keyboard_getchar();

    return c;

#else

    char c = fgetc(stdin);
    while (c == 0)
    {
        c = fgetc(stdin);
    }

    return c;

#endif
}
