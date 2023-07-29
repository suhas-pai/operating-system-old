/*
 * kernel/dev/init.c
 * © suhas pai
 */

#include "acpi/api.h"

#include "dtb/init.h"
#include "pci/pci.h"

#include "driver.h"

void serial_init() {
#if defined(__riscv) && defined(__LP64__)
    dtb_init_early();
#endif

    driver_foreach(iter) {
        switch (iter->kind) {
            case DRIVER_NONE:
                verify_not_reached();
            case DRIVER_UART:
                uart_init_driver(iter->uart_dev);
                break;
        }
    }
}

void dev_init() {
    acpi_init();
    dtb_init();
    pci_init();

    driver_foreach(iter) {
        switch (iter->kind) {
            case DRIVER_NONE:
                verify_not_reached();
            case DRIVER_UART:
                // Skip as we already initialized this earlier in serial_init()
                break;
        }
    }
}