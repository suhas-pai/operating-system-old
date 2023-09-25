/*
 * kernel/arch/riscv64/dev/init.c
 * © suhas pai
 */

#include "acpi/api.h"
#include "acpi/rhct.h"

#include "dev/printk.h"
#include "dev/uart/8250.h"

#include "mm/mm_types.h"
#include "mm/mmio.h"

static struct mmio_region *serial_mmio = NULL;

void arch_init_dev() {
    serial_mmio =
        vmap_mmio(range_create(0x10000000, PAGE_SIZE),
                  PROT_READ | PROT_WRITE,
                  /*flags=*/0);

    if (serial_mmio == NULL) {
        panic("dev: failed to init serial device\n");
    }

    uart8250_init((port_t)serial_mmio->base,
                  /*baudrate=*/115200,
                  /*in_freq=*/0,
                  /*reg_width=*/sizeof(uint8_t),
                  /*reg_shift=*/0);

    printk(LOGLEVEL_INFO, "riscv64: is console working?\n");

    const struct acpi_rhct *const rhct =
        (const struct acpi_rhct *)acpi_lookup_sdt("RHCT");

    if (rhct != NULL) {
        acpi_rhct_init(rhct);
    }
}