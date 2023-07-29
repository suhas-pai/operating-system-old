/*
 * kernel/arch/aarch64/dev/uart/p1011.c
 * © suhas pai
 */

#include "cpu/spinlock.h"
#include "dev/printk.h"
#include "mm/kmalloc.h"

#include "pl011.h"

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

static const uint32_t FR_BUSY = 1 << 3;

static const uint32_t CR_TXEN = 1 << 8;
static const uint32_t CR_UARTEN = 1 << 0;

static const uint32_t LCR_FEN = 1 << 4;
static const uint32_t LCR_STP2 = 1 << 3;

#define MAX_ATTEMPTS 10

struct pl011_device_info {
    struct terminal term;
    struct spinlock lock;

    volatile struct pl011_device *device;
};

// for use when initializing serial before mm/kmalloc
static struct pl011_device_info early_infos[64] = {};
static uint16_t early_info_count = 0;

static void wait_tx_complete(volatile const struct pl011_device *const dev) {
    for (uint64_t i = 0; i != MAX_ATTEMPTS; i++) {
        if ((dev->fr_offset & FR_BUSY) == 0) {
            return;
        }
    }
}

static void
pl011_send_char(struct terminal *const term,
                const char ch,
                const uint32_t amount)
{
    struct pl011_device_info *const info = (struct pl011_device_info *)term;
    volatile struct pl011_device *const device = info->device;

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
    struct pl011_device_info *const info = (struct pl011_device_info *)term;
    volatile struct pl011_device *const device = info->device;

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

#define PL011_BASE_CLOCK 0x16e3600

void
pl011_init(const port_t base,
           const uint32_t baudrate,
           const uint32_t data_bits,
           const uint32_t stop_bits)
{
    struct pl011_device_info *info = NULL;
    if (kmalloc_initialized()) {
        info = kmalloc(sizeof(*info));
        if (info == NULL) {
            printk(LOGLEVEL_WARN, "pl011: failed to alloc info\n");
            return;
        }
    } else {
        if (early_info_count == countof(early_infos)) {
            printk(LOGLEVEL_WARN, "pl011: exhausted early-infos struct\n");
            return;
        }

        info = &early_infos[early_info_count];
        early_info_count++;
    }

    volatile struct pl011_device *const device =
        (volatile struct pl011_device *)base;

    uint32_t cr = device->cr_offset;
    uint32_t lcr = device->lcr_offset;

    // Disable UART before anything else
    device->cr_offset = cr & CR_UARTEN;

    // Wait for any ongoing transmissions to complete
    wait_tx_complete(device);

    // Flush FIFOs
    device->lcr_offset = (lcr & ~LCR_FEN);

    // Set frequency divisors (UARTIBRD and UARTFBRD) to configure the speed
    const uint32_t div = 4 * PL011_BASE_CLOCK / baudrate;

    uint32_t ibrd = div & 0x3f;
    uint32_t fbrd = (div >> 6) & 0xffff;

    device->ibrd_offset = ibrd;
    device->fbrd_offset = fbrd;

    // Configure data frame format according to the parameters (UARTLCR_H).
    // We don't actually use all the possibilities, so this part of the code
    // can be simplified.
    lcr = 0x0;
    // WLEN part of UARTLCR_H, you can check that this calculation does the
    // right thing for yourself
    lcr |= ((data_bits - 1) & 0x3) << 5;

    // Configure the number of stop bits
    if (stop_bits == 2) {
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

    info->device = device;
    info->term.emit_ch = pl011_send_char,
    info->term.emit_sv = pl011_send_sv,

    printk_add_terminal(&info->term);
}