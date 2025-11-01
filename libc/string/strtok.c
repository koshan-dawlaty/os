#include <stdio.h>
#include <string.h>

char *strtok_r(char *s, char *sep, char **state)
{

    if (s == NULL)
    {

        if (*state == NULL)
        {
            return NULL;
        }

        s = *state;
    }

    size_t i = 0;
    while (s[i] != 0)
    {

        if (*sep == s[i])
        {

            char *out = s;
            out[i] = 0;

            *state = s + strlen(out) + 1;
            printf("returning:%p\n", out);

            return out;
        }

        i++;
    }

    *state = NULL;
    return s;
}

char *global_strtok_state;

char *strtok(char *s, char *sep)
{
    return strtok_r(s, sep, &global_strtok_state);
}
