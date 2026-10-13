#include <fcntl.h>
#include <lib/log.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv)
{

    if (argc <= 1)
    {
        loge("not enough args");
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0)
    {
        exit(1);
    }

    char buf[4] = {};
    if (read(fd, buf, 4) < 4)
    {
        /* could just be a very short text file */
        goto edit_open;
    }

    if ((buf[0] == 0x7F && buf[1] == 'E' && buf[2] == 'L' && buf[3] == 'F'))
    {
        execv(argv[1], (char *[]){argv[1], NULL});
    }
    else if (buf[0] == 0 && buf[1] == 0)
    {
        execv("/bin/imgview", (char *[]){"imgview", argv[1], NULL});
    }
    else if (*(u32 *)buf == 0x864ab572)
    {
        logw("cant open a font");
    }
    else
    {
    edit_open:
        logi("opening in editor as default");
        execv("/bin/edit", (char *[]){"edit", argv[1], NULL});
    }

    exit(0);
}
