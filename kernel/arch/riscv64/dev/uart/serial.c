/*
 * kernel/arch/riscv64/dev/uart/uart8250.c
 * © suhas pai
 */

#include "dev/driver.h"
#include "dev/uart/8250.h"

struct uart_driver uart8250_serial = {
    .base = (port_t)0x10000000,
    .baudrate = 115200,

    .init = uart8250_init,
    .extra_info = &(struct uart8250_info){
        .lock = SPINLOCK_INIT(),
        .in_freq = 3686400,
        .reg_width = 1,
        .reg_shift = 0
    }
};

EXPORT_UART_DRIVER(uart8250_serial);