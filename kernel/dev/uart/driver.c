/*
 * kernel/dev/uart/driver.c
 * © suhas pai
 */

#include "driver.h"

void uart_init_driver(struct uart_driver *const driver) {
    if (driver->init(driver)) {
        printk_add_terminal(&driver->term);
    }
}

void
uart_calculate_divisors(const struct uart_driver *const dev,
                        uint32_t *const fractional,
                        uint32_t *const integer)
{
    // 64 * F_UARTCLK / (16 * B) = 4 * F_UARTCLK / B
    const uint32_t div = 4 * dev->base_clock / dev->baudrate;

    *fractional = div & 0x3f;
    *integer = (div >> 6) & 0xffff;
}
