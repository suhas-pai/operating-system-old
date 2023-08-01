/*
 * kernel/dev/init.c
 * © suhas pai
 */

#if defined(__x86_64__)
    #include "dev/uart/com1.h"
#elif defined(__aarch64__)
    #include "dev/uart/pl011.h"
#endif /* defined(__x86_64__) */

#include "acpi/api.h"
#include "dtb/init.h"

#include "dev/printk.h"
#include "pci/pci.h"
#include "time/time.h"

#include "driver.h"

void serial_init() {
#if defined(__x86_64__)
    com1_init();
#elif defined(__aarch64__)
    pl011_init((port_t)phys_to_virt(0x9000000),
               /*baudrate=*/115200,
               /*data_bits=*/8,
               /*stop_bits=*/1);
#endif

    dtb_init_early();
}

void arch_init_dev();
void dev_init() {
    acpi_init();
    arch_init_time();

    dtb_init();
    pci_init();

    arch_init_dev();
    printk(LOGLEVEL_INFO,
           "dev: initialized time, seconds since boot: %" PRIu64 "\n",
           nano_to_seconds(nsec_since_boot()));
}