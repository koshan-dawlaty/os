#ifndef SERIAL_CDEV_H
#define SERIAL_CDEV_H

#include <types.h>

struct device *serial_cdev_create(u16 port);

#endif
