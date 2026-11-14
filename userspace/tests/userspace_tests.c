#include <errno.h>
#include <fcntl.h>
#include <lib/array.h>
#include <lib/utils.h>
#include <unistd.h>

#ifndef TESTS
#define TESTS
#endif
#include <tests/tests.h>

TEST(read_valid_addr, {
    int fd = open("/one", O_RDONLY);

    char *buf = malloc(10);

    ASSERT_GE(read(fd, buf, 123), 0);
    free(buf);

    close(fd);
})

int main()
{
    struct test_input input = {
        {0},
        TEST_INPUT_ONLY,
        /* TEST_INPUT_EXCLUDE, */
    };

    array_macro_append(&input.test_patterns, "*");

    tests_run_input(&input);
}
