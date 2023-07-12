/*
 * kernel/dev/uart/driver.h
 * © suhas pai
 */

#pragma once

#include <stdint.h>

#include "dev/printk.h"
#include "lib/adt/string_view.h"

enum uart_kind {
    UART_KIND_NONE,
#if defined(__x86_64__)
    UART_KIND_COM1
#elif defined(__aarch64__)
    UART_KIND_Pl011
#endif /* defined(__x86_64__) */
};

struct uart_driver {
    struct terminal term;

    void *device;
    enum uart_kind kind;

    uint64_t base_clock;
    uint32_t baudrate;
    uint32_t data_bits;
    uint32_t stop_bits;

    bool (*init)(struct uart_driver *);
};

void uart_init_driver(struct uart_driver *driver);

void
uart_calculate_divisors(const struct uart_driver *uart,
                        uint32_t *fraction,
                        uint32_t *integer);