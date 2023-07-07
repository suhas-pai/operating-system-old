/*
 * kernel/dev/uart/driver.c
 * © suhas pai
 */

#include "driver.h"

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
