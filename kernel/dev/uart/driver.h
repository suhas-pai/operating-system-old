/*
 * kernel/dev/uart/driver.h
 * © suhas pai
 */

#pragma once

#include "dev/printk.h"
#include "port.h"

enum uart_kind {
    UART_KIND_NONE,
    UART_KIND_8250,

#if defined(__aarch64__)
    UART_KIND_Pl011
#endif /* defined(__aarch64__) */
};

struct uart_driver {
    struct terminal term;

    void *device;
    enum uart_kind kind;

    port_t base;

    uint64_t base_clock;
    uint32_t baudrate;
    uint32_t data_bits;
    uint32_t stop_bits;

    bool (*init)(struct uart_driver *);
    void *extra_info;
};

void uart_init_driver(struct uart_driver *driver);

void
uart_calculate_divisors(const struct uart_driver *uart,
                        uint32_t *fraction,
                        uint32_t *integer);