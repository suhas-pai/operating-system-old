/*
 * kernel/dev/uart/8250.c
 * © suhas pai
 */

#include "8250.h"

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

static inline uint32_t
get_reg(const port_t uart8250_base,
        struct uart8250_info *const info,
        const uint32_t num)
{
    const uint32_t offset = num << info->reg_shift;
    if (info->reg_width == 1) {
        return port_in8(uart8250_base + offset);
    } else if (info->reg_width == 2) {
        return port_in16(uart8250_base + offset);
    }

    return port_in32(uart8250_base + offset);
}

static inline void
set_reg(const port_t uart8250_base,
        struct uart8250_info *const info,
        const uint32_t num,
        const uint32_t val)
{
    const uint32_t offset = num << info->reg_shift;
    if (info->reg_width == 1) {
        port_out8(uart8250_base + offset, val);
    } else if (info->reg_width == 2) {
        port_out16(uart8250_base + offset, val);
    } else {
        port_out32(uart8250_base + offset, val);
    }
}

static void
uart8250_putc(const port_t base,
              struct uart8250_info *const info,
              const char ch)
{
    while ((get_reg(base, info, UART_LSR_OFFSET) & UART_LSR_THRE) == 0) {}
    set_reg(base, info, UART_THR_OFFSET, ch);
}

static void
uart8250_send_char(struct terminal *const term,
                   const char ch,
                   const uint32_t amount)
{
    struct uart_driver *const driver = (struct uart_driver *)term;
    struct uart8250_info *const info =
        (struct uart8250_info *)driver->extra_info;

    const bool flag = spin_acquire_with_irq(&info->lock);
    for (uint32_t i = 0; i != amount; i++) {
        uart8250_putc(driver->base, info, ch);
    }

    spin_release_with_irq(&info->lock, flag);
}

static void
uart8250_send_sv(struct terminal *const term, const struct string_view sv) {
    struct uart_driver *const driver = (struct uart_driver *)term;
    struct uart8250_info *const info =
        (struct uart8250_info *)driver->extra_info;

    const bool flag = spin_acquire_with_irq(&info->lock);
    sv_foreach(sv, iter) {
        uart8250_putc(driver->base, info, *iter);
    }

    spin_release_with_irq(&info->lock, flag);
}

bool uart8250_init(struct uart_driver *const driver) {
    driver->term.emit_ch = uart8250_send_char,
    driver->term.emit_sv = uart8250_send_sv,
    driver->data_bits = 8;
    driver->stop_bits = 1;

    struct uart8250_info *const info =
        (struct uart8250_info *)driver->extra_info;

    uint16_t bdiv = 0;
    if (driver->baudrate != 0) {
        bdiv =
            (info->in_freq + 8 * driver->baudrate) / (16 * driver->baudrate);
    }

    /* Disable all interrupts */
    set_reg(driver->base, info, UART_IER_OFFSET, 0x00);
    /* Enable DLAB */
    set_reg(driver->base, info, UART_LCR_OFFSET, 0x80);

    if (bdiv != 0) {
        /* Set divisor low byte */
        set_reg(driver->base, info, UART_DLL_OFFSET, bdiv & 0xff);
        /* Set divisor high byte */
        set_reg(driver->base, info, UART_DLM_OFFSET, (bdiv >> 8) & 0xff);
    }

    /* 8 bits, no parity, one stop bit */
    set_reg(driver->base, info, UART_LCR_OFFSET, 0x03);
    /* Enable FIFO */
    set_reg(driver->base, info, UART_FCR_OFFSET, 0x01);
    /* No modem control DTR RTS */
    set_reg(driver->base, info, UART_MCR_OFFSET, 0x00);
    /* Clear line status */
    get_reg(driver->base, info, UART_LSR_OFFSET);
    /* Read receive buffer */
    get_reg(driver->base, info, UART_RBR_OFFSET);
    /* Set scratchpad */
    set_reg(driver->base, info, UART_SCR_OFFSET, 0x00);
    return true;
}