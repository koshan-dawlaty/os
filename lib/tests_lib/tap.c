#include "fcntl.h"
#include "tests/tests.h"
#include <extension.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PLAN_STR_START "1.."
#define STATUS_END " - "
#define STATUS_FAILED "not ok "
#define STATUS_SUCCEED "ok "

#define NAME_REASON_SEPARATOR ", "

char *tap_get_header(size_t num_tests)
{
    size_t output_len = strlen(PLAN_STR_START) + 1;
    char *output = malloc(output_len);
    memcpy(output, PLAN_STR_START, strlen(PLAN_STR_START));
    output[output_len - 1] = 0;

    char *num_str = rprintf("%d\n", num_tests);
    output_len += strlen(num_str);
    output = realloc(output, output_len);
    strcat(output, num_str);

    return output;
}

char *tap_format_test_result(test *t, test_result *r, size_t number)
{

    char *output = malloc(1);
    output[0] = 0;
    size_t output_len = 1;

    char *index_str = rprintf("%d", number + 1);

    const char *status;
    if (r->failed)
    {
        status = STATUS_FAILED;
    }
    else
    {
        status = STATUS_SUCCEED;
    }

    /* printf("index:%s\n", index_str); */

    /* printf("%d, %s\n", strlen(output), output); */

    output_len += strlen(status);
    output = realloc(output, output_len);
    strcat(output, status);

    /* printf("%d, %s\n", strlen(output), output); */

    /* printf("status:%s\n", status); */

    output_len += strlen(index_str);
    output = realloc(output, output_len);
    strcat(output, index_str);

    output_len += strlen(STATUS_END);
    output = realloc(output, output_len);
    strcat(output, STATUS_END);

    output_len += strlen(t->name);
    output = realloc(output, output_len);
    strcat(output, t->name);

    if (r->message != NULL && *r->message != 0)
    {

        output_len += strlen(NAME_REASON_SEPARATOR);
        output = realloc(output, output_len);
        strcat(output, NAME_REASON_SEPARATOR);

        output_len += strlen(r->message);
        output = realloc(output, output_len);
        strcat(output, r->message);
    }

    output_len++;
    output = realloc(output, output_len);
    strcat(output, "\n");

    return output;
}
