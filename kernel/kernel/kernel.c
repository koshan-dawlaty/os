#include <stddef.h>
#include <stdint.h>
#include <tty.h>

int main() {
    terminal_initialize();
    terminal_writestring("test");
}
