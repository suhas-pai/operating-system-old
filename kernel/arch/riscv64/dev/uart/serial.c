/*
 * kernel/arch/riscv64/dev/uart/serial.c
 * © suhas pai
 */

#include "dev/uart/8250.h"
#include "dev/driver.h"

struct uart_driver uart8250_serial = {
    .baudrate = 115200,
    .init = uart8250_init,
    .extra_info = &(struct uart8250_info){
        .lock = SPINLOCK_INIT(),
        .reg_width = 1,
        .reg_shift = 0
    }
};

EXPORT_UART_DRIVER(uart8250_serial);