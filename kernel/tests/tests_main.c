#include "memory/memory_tests.c"
#include "tests/tests.h"

void run_all_tests()
{
    run_all_arch_tests();

    run_memory_tests();
}
