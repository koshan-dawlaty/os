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

    ASSERT_GE(fd, 0, "fd for existing file should be valid");

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

        creat(pathname, 000);

        char *good_pathname = malloc(name_len + 1);
        memcpy(good_pathname + 1, pathname, strlen(pathname));
        good_pathname[strlen(pathname) + 1] = 0;
        good_pathname[0] = '/';

        int fd = open(pathname, 0);

        if (fd == -1)
        {
            FAIL_TEST();
        }

        free(pathname);
    }
})

TEST(vfs_create_dir, {
    char *path = "/dir/testfile";
    creat(path, 0);
    int fd1 = open(path, 0);

    ASSERT_GE(fd1, 0, "invalid fd:%d", fd1);
})

TEST(vfs_create_delete_dir, {
    char *testfile = "/three";
    creat(testfile, 0);
    int fd1 = open(testfile, 0);
    ASSERT_GE(fd1, 0, "opening newly created returns:%d", fd1);
    unlink(testfile);

    int fd2 = open(testfile, 0);
    ASSERT_LT(fd2, 0, "opening file after delete returns:%d", fd2);
    creat(testfile, 0);

    int fd3 = open(testfile, 0);
    ASSERT_GE(fd3, 0, "opening file again returns:%d", fd3);
})

TEST(vfs_doesnot_exist, {
    int fd = open("/doesnotexist", 0);
    ASSERT_LT(fd, 0, "fd for nonexistant file is %d", fd);
});

TEST(stdlib_FILE, {
    char *testfile = "/testfile12345678";
    creat(testfile, 0);

    size_t len = 100;

    FILE *f = fopen(testfile, "");

    char *write_buf = malloc(len);
    for (size_t i = 0; i < len; i++)
    {
        write_buf[i] = ((i % 26) + 'a');
    }

    fwrite(write_buf, 10, 10, f);

    rewind(f);

    char *buf = malloc(len);
    fread(buf, len, 1, f);

    for (size_t i = 0; i < len; i++)
    {
        if (buf[i] != write_buf[i])
        {
            FAIL_TEST("file at index:%d is not equal, %d != %d", i, buf[i],
                      write_buf[i]);
        }
    }
})
