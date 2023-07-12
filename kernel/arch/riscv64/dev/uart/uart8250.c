/*
 * kernel/arch/riscv64/dev/uart/uart8250.c
 * © suhas pai
 */

#include "dev/driver.h"
#include "port.h"

#define UART_RBR_OFFSET 0  /* In:  Recieve Buffer Register */
#define UART_THR_OFFSET 0  /* Out: Transmitter Holding Register */
#define UART_DLL_OFFSET 0  /* Out: Divisor Latch Low */
#define UART_IER_OFFSET 1  /* I/O: Interrupt Enable Register */
#define UART_DLM_OFFSET 1  /* Out: Divisor Latch High */
#define UART_FCR_OFFSET 2  /* Out: FIFO Control Register */
#define UART_IIR_OFFSET 2  /* I/O: Interrupt Identification Register */
#define UART_LCR_OFFSET 3  /* Out: Line Control Register */
#define UART_MCR_OFFSET 4  /* Out: Modem Control Register */
#define UART_LSR_OFFSET 5  /* In:  Line Status Register */
#define UART_MSR_OFFSET 6  /* In:  Modem Status Register */
#define UART_SCR_OFFSET 7  /* I/O: Scratch Register */
#define UART_MDR1_OFFSET 8  /* I/O:  Mode Register */

#define UART_LSR_FIFOE 0x80 /* Fifo error */
#define UART_LSR_TEMT 0x40  /* Transmitter empty */
#define UART_LSR_THRE 0x20  /* Transmit-hold-register empty */
#define UART_LSR_BI 0x10    /* Break interrupt indicator */
#define UART_LSR_FE 0x08    /* Frame error indicator */
#define UART_LSR_PE 0x04    /* Parity error indicator */
#define UART_LSR_OE 0x02    /* Overrun error indicator */
#define UART_LSR_DR 0x01    /* Receiver data ready */
#define UART_LSR_BRK_ERROR_BITS 0x1E    /* BI, FE, PE, OE bits */

static uint32_t uart8250_in_freq = 3686400;
static uint32_t uart8250_baudrate = 115200;
static uint32_t uart8250_reg_width = 1;
static uint32_t uart8250_reg_shift = 0;

static inline
uint32_t get_reg(volatile void *const uart8250_base, const uint32_t num) {
    const uint32_t offset = num << uart8250_reg_shift;
    if (uart8250_reg_width == 1) {
        return port_in8(uart8250_base + offset);
    } else if (uart8250_reg_width == 2) {
        return port_in16(uart8250_base + offset);
    }

    return port_in32(uart8250_base + offset);
}

static inline void
set_reg(volatile void *const uart8250_base,
        const uint32_t num,
        const uint32_t val)
{
    const uint32_t offset = num << uart8250_reg_shift;
    if (uart8250_reg_width == 1) {
        port_out8(uart8250_base + offset, val);
    } else if (uart8250_reg_width == 2) {
        port_out16(uart8250_base + offset, val);
    } else {
        port_out32(uart8250_base + offset, val);
    }
}

static void uart8250_putc(volatile void *const uart8250_base, const char ch) {
    while ((get_reg(uart8250_base, UART_LSR_OFFSET) & UART_LSR_THRE) == 0) {}
    set_reg(uart8250_base, UART_THR_OFFSET, ch);
}

static void
uart8250_send_char(struct terminal *const term,
                   const char ch,
                   const uint32_t amount)
{
    struct uart_driver *const driver = (struct uart_driver *)term;
    for (uint32_t i = 0; i != amount; i++) {
        uart8250_putc((volatile void *)driver->device, ch);
    }
}

static void
uart8250_send_sv(struct terminal *const console, const struct string_view sv) {
    struct uart_driver *const driver = (struct uart_driver *)console;
    sv_foreach(sv, iter) {
        uart8250_putc((volatile void *)driver->device, *iter);
    }
}

static bool uart8250_init(struct uart_driver *const uart) {
    uart->device = (void *)0x10000000 + /*reg_offset=*/0;
    volatile void *uart8250_base = uart->device;

    uint16_t bdiv = 0;
    if (uart8250_baudrate != 0) {
        bdiv =
            (uart8250_in_freq + 8 * uart8250_baudrate) /
            (16 * uart8250_baudrate);
    }

    /* Disable all interrupts */
    set_reg(uart8250_base, UART_IER_OFFSET, 0x00);
    /* Enable DLAB */
    set_reg(uart8250_base, UART_LCR_OFFSET, 0x80);

    if (bdiv) {
        /* Set divisor low byte */
        set_reg(uart8250_base, UART_DLL_OFFSET, bdiv & 0xff);
        /* Set divisor high byte */
        set_reg(uart8250_base, UART_DLM_OFFSET, (bdiv >> 8) & 0xff);
    }

    /* 8 bits, no parity, one stop bit */
    set_reg(uart8250_base, UART_LCR_OFFSET, 0x03);
    /* Enable FIFO */
    set_reg(uart8250_base, UART_FCR_OFFSET, 0x01);
    /* No modem control DTR RTS */
    set_reg(uart8250_base, UART_MCR_OFFSET, 0x00);
    /* Clear line status */
    get_reg(uart8250_base, UART_LSR_OFFSET);
    /* Read receive buffer */
    get_reg(uart8250_base, UART_RBR_OFFSET);
    /* Set scratchpad */
    set_reg(uart8250_base, UART_SCR_OFFSET, 0x00);
    return true;
}

struct uart_driver uart8250_serial = {
    .term.emit_ch = uart8250_send_char,
    .term.emit_sv = uart8250_send_sv,

    .baudrate = 115200,
    .data_bits = 8,
    .stop_bits = 1,
    .init = uart8250_init,
};

EXPORT_UART_DRIVER(uart8250_serial);