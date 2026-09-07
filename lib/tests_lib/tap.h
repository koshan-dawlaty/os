#ifndef TAP_TEST_H
#define TAP_TEST_H

#include "tests/tests.h"
#include <stddef.h>

char *tap_format_test_result(test *t, test_result *r, size_t number);
char *tap_get_header(size_t num_tests);

#endif
