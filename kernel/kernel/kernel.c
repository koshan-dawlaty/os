#include "multiboot.h"
#include "paging.h"
#include <kernel/tty.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

int main()
{

    terminal_initialize();

    init_paging();

    return 0;
}
