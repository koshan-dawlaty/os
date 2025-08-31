#include "tests/tests.h"
#include "unistd.h"
#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>
#include <vfs.h>

TEST(vfs_basic, {
    size_t size = 1000;
    char *buf = malloc(size);
    for (size_t i = 0; i < size; i++)
    {
        buf[i] = (i + 0x30) % 10;
    }

    int fd = open("/testdir/testfile", 0);

    pwrite(fd, buf, size, 0);

    char *read_buf = malloc(size);
    pread(fd, read_buf, size, 0);

    for (size_t i = 0; i < size; i++)
    {
        ASSERT_EQ(read_buf[i], buf[i]);
    }
})
