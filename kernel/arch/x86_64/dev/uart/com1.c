/*
 * kernel/arch/x86_64/dev/uart/com1.c
 * © suhas pai
 */

#include "dev/driver.h"
#include "dev/uart/8250.h"

#define COM1 0x3f8
__unused static bool com1_init() {
    port_out8(COM1 + 1, 0x00);    // Disable all interrupts
    port_out8(COM1 + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    port_out8(COM1 + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
    port_out8(COM1 + 1, 0x00);    //                  (hi byte)
    port_out8(COM1 + 3, 0x03);    // 8 bits, no parity, one stop bit
    port_out8(COM1 + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
    port_out8(COM1 + 4, 0x0B);    // IRQs enabled, RTS/DSR set
    port_out8(COM1 + 4, 0x1E);    // Set in loopback mode, test the serial chip
    port_out8(COM1 + 0, 0xAE);    // Test serial chip (send byte 0xAE and check if serial returns same byte)

    // Check if serial is faulty (i.e: not same byte as sent)
    if (port_in8(COM1 + 0) != 0xAE) {
        return false;
    }

    // If serial is not faulty set it in normal operation mode
    // (not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled)

    port_out8(COM1 + 4, 0x0F);
    return true;
}

struct uart_driver com1_serial = {
    .base = COM1,
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