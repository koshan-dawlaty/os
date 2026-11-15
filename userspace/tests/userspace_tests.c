#include <errno.h>
#include <fcntl.h>
#include <lib/array.h>
#include <lib/log.h>
#include <lib/utils.h>
#include <unistd.h>

#ifndef TESTS
#define TESTS
#endif
#include <tests/tests.h>

TEST(read_valid_addr, {
    int fd = open("/one", O_RDONLY);

    char *buf = malloc(10);
    ASSERT_GE(read(fd, buf, 10), 0);
    free(buf);

    close(fd);
})

TEST(read_invalid_addr, {
    int fd = open("/one", O_RDONLY);
    ASSERT_EQ(read(fd, NULL, 123), -EFAULT);
    ASSERT_EQ(read(fd, (void *)-1, 123), -EFAULT);
    ASSERT_EQ(read(fd, (void *)-1000, 123), -EFAULT);
    close(fd);
})

TEST(write_valid_addr, {
    int fd = open("/one", O_RDWR);
    char *buf = "abc";
    ASSERT_GE(write(fd, buf, strlen(buf)), 0);
    close(fd);
})

TEST(write_invalid_addr, {
    int fd = open("/one", O_RDWR);
    ASSERT_EQ(write(fd, NULL, 123), -EFAULT);
    ASSERT_EQ(write(fd, (void *)-1, 123), -EFAULT);
    ASSERT_EQ(write(fd, (void *)-1000, 123), -EFAULT);
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
