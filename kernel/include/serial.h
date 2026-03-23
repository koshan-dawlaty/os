#ifndef SERIAL_H
#define SERIAL_H

#include <stddef.h>
#include <types.h>

void serial_write(u16 port, u8 data);
void serial_init(u16 port);
void serial_writestring(const char *str, size_t len, u16 port);
u8 serial_read(u16 port);

#endif
