#include <string.h>

int memcmp(const void *s1, const void *s2, size_t n)
{

    const u8 *one = s1;
    const u8 *two = s2;
    for (size_t i = 0; i < n; i++)
    {

        if (one[i] != two[i])
        {
            return (int)one[i] - (int)two[i];
        }
    }
	return 0;
}
