#ifndef TAP_TEST_H
#define TAP_TEST_H

#include "tests/tests.h"
#include <stddef.h>

char *format_test_results_tap(test **tests, test_result **results,
                              size_t num_tests);

#endif
