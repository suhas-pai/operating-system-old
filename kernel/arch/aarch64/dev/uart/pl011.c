/*
 * kernel/arch/aarch64/dev/uart/p1011.c
 * © suhas pai
 */

#include "dev/driver.h"

struct pl011_device {
    volatile uint32_t dr_offset;

    volatile uint32_t padding[5];
    volatile uint32_t fr_offset;

    volatile uint64_t padding_2;
    volatile uint32_t ibrd_offset;
    volatile uint32_t fbrd_offset;
    volatile uint32_t lcr_offset;
    volatile uint32_t cr_offset;

    volatile uint32_t padding_3;
    volatile uint32_t imsc_offset;

    volatile uint32_t padding_4[3];
    volatile uint32_t dmacr_offset;
} __packed;

_Static_assert(offsetof(struct pl011_device, dr_offset) == 0, "");
_Static_assert(offsetof(struct pl011_device, fr_offset) == 0x018, "");
_Static_assert(offsetof(struct pl011_device, ibrd_offset) == 0x024, "");
_Static_assert(offsetof(struct pl011_device, fbrd_offset) == 0x028, "");
_Static_assert(offsetof(struct pl011_device, lcr_offset) == 0x02c, "");
_Static_assert(offsetof(struct pl011_device, cr_offset) == 0x030, "");
_Static_assert(offsetof(struct pl011_device, imsc_offset) == 0x038, "");
_Static_assert(offsetof(struct pl011_device, dmacr_offset) == 0x048, "");

static const uint32_t FR_BUSY = (1 << 3);

static const uint32_t CR_TXEN = (1 << 8);
static const uint32_t CR_UARTEN = (1 << 0);

static const uint32_t LCR_FEN = (1 << 4);
static const uint32_t LCR_STP2 = (1 << 3);

static void wait_tx_complete(const struct pl011_device *const dev) {
    while ((dev->fr_offset & FR_BUSY) != 0) {}
}

static void
pl011_send_char(struct terminal *const term,
                const char ch,
                const uint32_t amount)
{
    struct uart_driver *const uart = (struct uart_driver *)term;
    struct pl011_device *const device = uart->device;

    wait_tx_complete(device);
    if (ch == '\n') {
        for (uint64_t i = 0; i != amount; i++) {
            device->dr_offset = '\r';
            wait_tx_complete(device);

            device->dr_offset = ch;
            wait_tx_complete(device);
        }
    } else {
        for (uint64_t i = 0; i != amount; i++) {
            device->dr_offset = ch;
            wait_tx_complete(device);
        }
    }
}

static void
pl011_send_sv(struct terminal *const term, const struct string_view sv) {
    struct uart_driver *const uart = (struct uart_driver *)term;
    struct pl011_device *const device = uart->device;

    wait_tx_complete(device);
    sv_foreach(sv, iter) {
        char ch = *iter;
        if (ch == '\n') {
            device->dr_offset = '\r';
            wait_tx_complete(device);
        }

        device->dr_offset = ch;
        wait_tx_complete(device);
    }
}

static void pl011_reset(const struct uart_driver *const uart) {
    struct pl011_device *const device = uart->device;

    uint32_t cr = device->cr_offset;
    uint32_t lcr = device->lcr_offset;

    // Disable UART before anything else
    device->cr_offset = cr & CR_UARTEN;

    // Wait for any ongoing transmissions to complete
    wait_tx_complete(device);

    // Flush FIFOs
    device->lcr_offset = (lcr & ~LCR_FEN);

    // Set frequency divisors (UARTIBRD and UARTFBRD) to configure the speed
    uint32_t ibrd = 0;
    uint32_t fbrd = 0;

    uart_calculate_divisors(uart, &ibrd, &fbrd);

    device->ibrd_offset = ibrd;
    device->fbrd_offset = fbrd;

    // Configure data frame format according to the parameters (UARTLCR_H).
    // We don't actually use all the possibilities, so this part of the code
    // can be simplified.
    lcr = 0x0;
    // WLEN part of UARTLCR_H, you can check that this calculation does the
    // right thing for yourself
    lcr |= ((uart->data_bits - 1) & 0x3) << 5;

    // Configure the number of stop bits
    if (uart->stop_bits == 2) {
        lcr |= LCR_STP2;
    }

    // Mask all interrupts by setting corresponding bits to 1
    device->imsc_offset = 0x7ff;

    // Disable DMA by setting all bits to 0
    device->dmacr_offset = 0x0;

    // I only need transmission, so that's the only thing I enabled.
    device->cr_offset = CR_TXEN;

    // Finally enable UART
    device->cr_offset = CR_TXEN | CR_UARTEN;
}

static bool pl011_init(struct uart_driver *const uart) {
    uart->device = (void *)0x9000000;
    uart->base_clock = 0x16e3600;

    pl011_reset(uart);
    return true;
}

struct uart_driver pl011_serial = {
    .term.emit_ch = pl011_send_char,
    .term.emit_sv = pl011_send_sv,

    .baudrate = 115200,
    .data_bits = 8,
    .stop_bits = 1,
    .init = pl011_init,
};

EXPORT_UART_DRIVER(pl011_serial);