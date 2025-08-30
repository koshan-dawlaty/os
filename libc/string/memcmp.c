#include <string.h>

int memcmp(void *arg_one, void *arg_two, size_t n)
{

    char *one = arg_one;
    char *two = arg_two;

    for (size_t i = 0; i < n; i++)
    {
        if (one[i] < two[i])
        {
            return -1;
        }
        else if (one[i] > two[i])
        {
            return 1;
        }
    }
    return 0;
}
