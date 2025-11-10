#include "tests/tests.h"
#include "unistd.h"
#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
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

    free(read_buf);
    free(buf);
})

TEST(vfs_create_basic, {
    creat("testfile1234", 0);
    if (open("/testfile1234", 0) == -1)
    {
        FAIL_TEST("could not open /testfile1234");
    }
})

/* seems to not work */
TEST(vfs_create, {
    size_t name_len = 2;
    char *name = malloc(name_len);
    name[0] = 'a';
    name[1] = 0;

    const char *literal = "testfile123";

    for (size_t i = 0; i < 1; i++)
    {

        char *pathname = malloc(strlen(name) + strlen(literal) + 1);
        memcpy(pathname, literal, strlen(literal));
        memcpy(pathname + strlen(literal), name, strlen(name));
        pathname[strlen(literal) + strlen(name)] = 0;

        name_len++;
        name = realloc(name, name_len);
        /* name[name_len - 2] = i + 0x60; */
        /* name[name_len - 1] = 0; */

        printf("pathname is:%s\n", pathname);
        printf("should be zero:%p\n", pathname[strlen(literal) + 1]);

        creat(pathname, 000);

        char *good_pathname = malloc(name_len + 1);
        memcpy(good_pathname + 1, pathname, strlen(pathname));
        good_pathname[strlen(pathname) + 1] = 0;
        good_pathname[0] = '/';

        printf("reading:%s\n", good_pathname);

        int fd = open(pathname, 0);

        if (fd == -1)
        {
            FAIL_TEST();
        }

        free(pathname);
    }
})
