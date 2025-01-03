#include "tests/tests.h"
#include <stdarg.h>

void test_debug_print(char *format, ...)
{
#ifdef TEST_DEBUG

    va_list parameters;
    va_start(parameters, format);

    vprintf(format, parameters);

    va_end(parameters);
#endif
}
