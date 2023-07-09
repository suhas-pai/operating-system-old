/*
 * kernel/dev/driver.h
 * © suhas pai
 */

#pragma once
#include "kernel/dev/uart/driver.h"

enum driver_kind {
    DRIVER_NONE,
    DRIVER_UART,
};

struct driver {
    enum driver_kind kind;
    union {
        struct uart_driver *uart_dev;
    };
};

extern struct driver drivers_start;
extern struct driver drivers_end;

#define driver_foreach(iter) \
    for (struct driver *iter = &drivers_start; \
         iter < &drivers_end;                  \
         iter++)                               \

#define EXPORT_UART_DRIVER(drv) \
    __attribute__((used, section(".drivers"))) \
    static struct driver __##drv = { .kind = DRIVER_UART, .uart_dev = &drv }
