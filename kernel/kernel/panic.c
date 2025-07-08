#include "kernel/tty.h"
#include "vga.h"
#include <stdarg.h>
#include <stdio.h>

extern void panic_asm();

/* TODO: maybe print register states? */
void panic(char *message, ...)
{
    va_list parameters;
    va_start(parameters, message);

    asm("cli");
    panic_asm();

    printf("kernel panicked\n");
    printf("reason:\n");
    vprintf(message, parameters);

    /* because this does not return i dont know where to put this */
    va_end(parameters);

    while (1)
    {
        asm("hlt");
    }
}
