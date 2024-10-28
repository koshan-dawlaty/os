#include <string.h>

size_t strlen(char *str) {
    size_t len = 0;
    while (*str != '\0') {
        len++;
        str++;
    }

    return len;
}
