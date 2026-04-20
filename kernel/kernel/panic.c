#include "kernel/tty.h"
#include "lib/log.h"
#include "utils.h"
#include "vga.h"
#include <extension.h>
#include <panic.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

extern void panic_asm();

/* TODO: maybe print register states? */
/* void panic(const char *message, ...) */
/* { */

/*     va_list parameters; */
/*     va_start(parameters, message); */

/*     vpanic(message, parameters); */

/*     /1* because this does not return i dont know where to put this *1/ */
/*     va_end(parameters); */
/* } */

#define KERNEL_PANIC_MESSAGE                                                   \
    "kernel panicked\n"                                                        \
    "reason:\n"

void vpanic(const char *message, va_list parameters)
{
    char *str = vrprintf(message, parameters);
    loge(KERNEL_PANIC_MESSAGE "%s", str);
    panic_no_malloc(str);
}

void panic_no_malloc(const char *message)
{

    irq_disable();
    panic_asm();

    terminal_writestring(KERNEL_PANIC_MESSAGE);
    terminal_writestring(message);

    while (1)
    {
        asm("hlt");
    }
}
