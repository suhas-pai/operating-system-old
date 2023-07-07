/*
 * kernel/dev/init.c
 * © suhas pai
 */

#include "acpi/api.h"
#include "dev/printk.h"

#include "driver.h"
#include "init.h"


static void init_uart(struct uart_driver *const driver) {
    driver->init(driver);
    printk_add_console(&driver->console);
}

void serial_init() {
    driver_foreach(iter) {
        switch (iter->kind) {
            case DRIVER_NONE:
                verify_not_reached();
            case DRIVER_UART:
                init_uart(iter->uart_dev);
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