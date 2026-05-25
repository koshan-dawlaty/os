#include "lib/log.h"
#include "sys/syscall.h"
#include <errno.h>
#include <fcntl.h>
#include <kernel_syscall.h>
#include <stdint.h>
#include <string.h>
#include <syscall.h>
#include <unistd.h>
#include <utils.h>

#ifdef __is_libk
#include <elf.h>
#include <scheduler.h>
#endif

int execv(const char *path, char *const argv[])
{
#ifndef __is_libk
    return syscall_wrapper_asm(SYS_execv, (uintptr_t)path, (uintptr_t)argv, 0,
                               0, 0);

#else

    logd("execv start");

    /* TODO: wrong oflags */
    int f = open(path, O_RDONLY);

    if (f < 0)
    {
        logd("%s, %s", path, strerror(f));
        return -ENOENT;
    }

    elf *elf = parse_elf(f);
    if (elf != NULL)
    {

        int argc = 0;
        /* printf("%p\n", argv[0]); */
        while (argv[argc] != NULL)
        {
            argc++;
        }
        logd("loading elf with argc as:%d", argc);

        struct task *t = load_elf(elf, argc, (char **)argv);
        add_task(t);
        free_elf(elf);
        logd("execv done");
        /* this the old task from running soon after this is called, it also
         * makes this function more responsive as the new task should be
         * immediately switched to
         */
        /* schedule(); */
    }
    else
    {
        return -ENOEXEC;
    }

    close(f);

    return 0;
#endif
}

SYSCALL_DEFINE2(SYS_execv, int, const char *, path, char **, argv)
{
    return execv(path, argv);
}
#ifdef __is_libk
int execv_stdio(const char *path, char *const argv[], int stdin, int stdout)
{

    struct fdlist *current_fdlist = get_current_task()->descriptors;
    struct file *new_stdin = fdlist_lookup(current_fdlist, stdin);
    struct file *new_stdout = fdlist_lookup(current_fdlist, stdout);

    /* TODO: wrong oflags */
    int f = open((char *)path, O_RDONLY);

    if (f < 0)
    {
        return -ENOENT;
    }

    elf *elf = parse_elf(f);
    if (elf != NULL)
    {

        int argc = 0;
        /* printf("%p\n", argv[0]); */
        while (argv[argc] != NULL)
        {
            argc++;
        }
        struct task *t = load_elf(elf, argc, (char **)argv);

        /* TODO: this does not free what was in the array, make a scheduler
         * function that takes stdin and stdout as arguments  */
        t->descriptors->files[0] = new_stdin;
        t->descriptors->files[1] = new_stdout;
        add_task(t);
        free_elf(elf);
        /* this the old task from running soon after this is called, it
         * also makes this function more responsive as the new task
         * should be immediately switched to
         */
        /* schedule(); */
    }
    else
    {
        return -ENOEXEC;
    }

    close(f);

    return 0;
}
#endif

/* TODO: */
/* SYSCALL_DEFINE4(execv_stdio, SYS_execv_stdio); */

SYSCALL_DEFINE4(SYS_execv_stdio, int, const char *, path, char **, argv, int,
                stdin, int, stdout)
{
#ifdef __is_libk
    return execv_stdio(path, argv, stdin, stdout);
#endif

    return 0;
}
