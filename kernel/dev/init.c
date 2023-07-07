/*
 * kernel/dev/init.c
 * © suhas pai
 */

#include "acpi/api.h"
#include "dev/printk.h"

#include "driver.h"
#include "init.h"

void serial_init() {
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