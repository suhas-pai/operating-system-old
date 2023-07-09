/*
 * kernel/arch/x86_64/dev/uart/com1.c
 * © suhas pai
 */

#include "cpu/spinlock.h"
#include "cpu/util.h"

#include "dev/driver.h"
#include "dev/printk.h"

#include "port.h"

#define COM1 0x3f8

static void com1_out(const char ch) {
    while ((port_in8(COM1 + 5) & 0x20) == 0) {}
    port_out8(COM1, ch);
}

static struct spinlock g_spinlock = {};

static void
com1_write_char(struct console *const console,
                const char ch,
                const uint32_t amount)
{
    (void)console;

    const int flag = spin_acquire_with_irq(&g_spinlock);
    for (uint64_t i = 0; i != amount; i++) {
        com1_out(ch);
    }

    spin_release_with_irq(&g_spinlock, flag);
}

static
void com1_write_sv(struct console *const console, const struct string_view sv) {
    (void)console;

    const int flag = spin_acquire_with_irq(&g_spinlock);
    sv_foreach(sv, iter) {
        com1_out(*iter);
    }

    spin_release_with_irq(&g_spinlock, flag);
}

static void com1_bust_locks(struct console *const console) {
    (void)console;
    g_spinlock = (struct spinlock){};
}

static bool com1_init() {
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
    .console.emit_ch = com1_write_char,
    .console.emit_sv = com1_write_sv,
    .console.bust_locks = com1_bust_locks,

    .kind = UART_KIND_COM1,
    .base_clock = 0,
    .baudrate = 38400,
    .data_bits = 8,
    .stop_bits = 1,

    .init = com1_init
};

EXPORT_UART_DRIVER(com1_serial);