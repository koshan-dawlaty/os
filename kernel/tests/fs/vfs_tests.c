#include "panic.h"
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <tests/tests.h>
#include <unistd.h>
#include <vfs.h>

TEST(vfs_basic, {
    size_t size = 1000;
    char *buf = malloc(size);
    for (size_t i = 0; i < size; i++)
    {
        buf[i] = (i + 0x30) % 10;
    }

    int fd = open("/testdir/testfile", O_RDWR);

    ASSERT_GE(fd, 0, "fd for existing file should be valid");

    pwrite(fd, buf, size, 0);

    char *read_buf = malloc(size);
    int read_result = pread(fd, read_buf, size, 0);
    ASSERT_EQ(read_result, (int)size, "read %d bytes when %d were expected",
              read_result, size);

    for (size_t i = 0; i < size; i++)
    {
        ASSERT_EQ(read_buf[i], buf[i]);
    }

    free(read_buf);
    free(buf);

    close(fd);
})

TEST(vfs_create_basic, {
    char *pathname = "/testfile1234";
    creat(pathname, 0);
    int fd = open(pathname, 0);
    if (fd < 0)
    {
        FAIL_TEST("could not open /testfile1234");
    }

    close(fd);
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

        close(fd);
    }
})

TEST(vfs_create_dir, {
    char *path = "/dir/testfile";
    creat(path, 0);
    int fd1 = open(path, 0);

    ASSERT_GE(fd1, 0, "invalid fd:%d", fd1);

    close(fd1);
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

    close(fd1);
    close(fd2);
    close(fd3);
})

TEST(vfs_doesnot_exist, {
    int fd = open("/doesnotexist", 0);
    ASSERT_LT(fd, 0, "fd for nonexistant file is %d", fd);
});

TEST(stdlib_FILE, {
    char *testfile = "/testfile12345678";
    creat(testfile, 0);

    size_t len = 100;

    FILE *f = fopen(testfile, "r+");

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

TEST(vfs_read_dir, {
    /* this should use O_SEARCH */
    /* struct file *f = open_file("/", O_SEARCH); */
    struct file *f = open_file("/", O_RDWR);

    size_t num = 2;
    /* we want this to be zeroed because we check if the name is null later */
    struct dirent *buf = calloc(num, sizeof(struct dirent));
    read_dir(f, buf, num);

    for (size_t i = 0; i < num; i++)
    {
        ASSERT_NONULL(buf[i].d_name,
                      "element name in buffer should not be null");
    }
})

TEST(vfs_dir_dot_dot_on_root_is_root, {
    int fd = open("/../../../../../one", O_RDONLY);
    ASSERT_GE(fd, 0, "file descriptor should be valid");

    char *buf = malloc(100);
    read(fd, buf, 100);

    const char *expected = "file one\n";
    ASSERT_EQ_STR(buf, expected, "%s != %s", buf, expected);

    close(fd);
})

TEST(vfs_dir_dot_dot_on_dir, {
    int fd = open("/dir/../dir/../dir/../dir/../one", O_RDONLY);
    ASSERT_GE(fd, 0, "file descriptor should be valid");
    char *buf = malloc(100);
    read(fd, buf, 100);

    const char *expected = "file one\n";
    ASSERT_EQ_STR(buf, expected, "%s != %s", buf, expected);

    close(fd);
})

TEST(vfs_mkdir, {
    mkdir("test1234567", 0);
    creat("test1234567/file", 0);
    int fd = open("/test1234567/file", 0);
    ASSERT_GE(fd, 0);
    close(fd);
})

TEST(vfs_file_permissions, {
    int write_only = open("/one", O_WRONLY);
    char *buf = malloc(100);
    int err = read(write_only, buf, 100);
    ASSERT_EQ(-err, EACCES, "reading from a WRONLY file should not work")
    close(write_only);
    int read_only = open("/one", O_RDONLY);
    err = write(read_only, buf, 100);
    ASSERT_EQ(-err, EACCES, "writing to a RDONLY file should not work")
})
