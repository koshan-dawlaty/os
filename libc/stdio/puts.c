#include <stddef.h>
#include <stdio.h>
#include <string.h>

#ifdef __is_libk
#include <spinlock.h>
struct spinlock writestring_spinlock = SPINLOCK_INIT();
#endif

int puts(char *str)
{

#ifdef __is_libk
    /* TODO: on non-smp builds this will compile to just disabling preemption,
     * it might be a good idea to not do that to speed up things */
    spinlock_lock(&writestring_spinlock);
    size_t i = 0;
    while (str[i] != 0)
    {
        putchar(str[i]);
        i++;
    }
    spinlock_unlock(&writestring_spinlock);
    return 0;
#else

    /* dont write the null terminator */
    fwrite(str, strlen(str), 1, stdout);
    return 0;
#endif
}
