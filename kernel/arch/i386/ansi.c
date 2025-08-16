#include "ansi.h"
#include "kernel/tty.h"
#include <stdbool.h>
#include <stdio.h>
/* TODO: bad */

bool parsed_2 = false;

bool parse_ansi(char c)
{
    if (c == '[')
    {
        return false;
    }
    if (c == '2')
    {
        parsed_2 = true;
        return false;
    }
    if (parsed_2)
    {
        parsed_2 = false;
        if (c == 'J')
        {
            printf("test\n");
            terminal_clear();
        }
        return true;
    }
    return true;
}
