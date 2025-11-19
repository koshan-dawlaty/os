#include "kernel/tty.h"
#include "vga.h"
#include <extension.h>
#include <panic.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

extern void panic_asm();

/* TODO: maybe print register states? */
void panic(const char *message, ...)
{

    va_list parameters;
    va_start(parameters, message);

    vpanic(message, parameters);

    /* because this does not return i dont know where to put this */
    va_end(parameters);
}

void vpanic(const char *message, va_list parameters)
{
    char *str = vrprintf(message, parameters);
    panic_no_malloc(str);
}

void panic_no_malloc(const char *message)
{

    asm("cli");
    panic_asm();

    terminal_writestring("kernel panicked\n");
    terminal_writestring("reason:\n");
    terminal_writestring(message);

    while (1)
    {
        asm("hlt");
    }
}
