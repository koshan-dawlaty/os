#include <stdlib.h>

int char_to_int(char in) {
    return in + 0x30;
}

int atoi(char *in) {
    int i = 0;
    int output = 0;

    while (in[i] != '\0') {

        output += char_to_int(in[i]) * (10 * i);

        i++;
    }

    return output;
}
