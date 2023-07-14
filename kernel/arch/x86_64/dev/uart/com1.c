/*
 * kernel/arch/x86_64/dev/uart/com1.c
 * © suhas pai
 */

#include "dev/driver.h"
#include "dev/uart/8250.h"

struct uart_driver com1_serial = {
    .base = 0x3f8,
    .baudrate = 38400,

    .init = uart8250_init,
    .extra_info = &(struct uart8250_info) {
        .lock = SPINLOCK_INIT(),
        .in_freq = 1536000,
        .reg_shift = 0,
        .reg_width = 1
    }
};

EXPORT_UART_DRIVER(com1_serial);