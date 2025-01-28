#include <stdio.h>

int sprintf(char *s, const char *format, ...)
{
    va_list parameters;
    va_start(parameters, format);

    int result = vsprintf(s, format, parameters);

    va_end(parameters);

    return result;
}
