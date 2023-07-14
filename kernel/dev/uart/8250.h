/*
 * kernel/dev/uart/8250.h
 * © suhas pai
 */

#pragma once

#include "cpu/spinlock.h"
#include "dev/uart/driver.h"

struct uart8250_info {
    struct spinlock lock;

    uint32_t in_freq;
    uint32_t reg_width;
    uint32_t reg_shift;
};

bool uart8250_init(struct uart_driver *driver);