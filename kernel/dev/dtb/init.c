/*
 * kernel/dev/dtb/init.c
 * © suhas pai
 */

#include "dev/pci/pci.h"
#include "dev/uart/8250.h"

#include "fdt/libfdt.h"

#include "dtb.h"
#include "limine.h"

static volatile struct limine_dtb_request dtb_request = {
    .id = LIMINE_DTB_REQUEST,
    .revision = 0,
    .response = NULL
};

#if defined(__riscv) && defined(__LP64__)
    static void init_riscv_serial_drivers(const void *const dtb) {
        extern struct uart_driver uart8250_serial;
        const int serial_offset =
            fdt_node_offset_by_compatible(dtb, -1, "ns16550a");

        if (serial_offset < 0) {
            panic("dtb: serial \"ns16550a\" node is not found");
        }

        struct dtb_reg_entry base_addr_reg = {0};
        uint32_t entry_count = 1;

        const bool get_base_addr_reg_result =
            dtb_get_reg_entries(dtb,
                                serial_offset,
                                /*start_index=*/0,
                                &entry_count,
                                &base_addr_reg);

        if (!get_base_addr_reg_result) {
            panic("dtb: base-addr reg of 'reg' property of serial node is "
                  "malformed\n");
        }

        struct string_view clock_freq_string = {};
        const bool get_clock_freq_result =
            dtb_get_string_prop(dtb,
                                serial_offset,
                                "clock-frequency",
                                &clock_freq_string);

        if (!get_clock_freq_result) {
            panic("dtb: clock-frequency property of serial node is missing or "
                  "malformed");
        }

        uart8250_serial.base = (port_t)base_addr_reg.address;
        ((struct uart8250_info *)uart8250_serial.extra_info)->in_freq =
            *(uint32_t *)(uint64_t)clock_freq_string.begin;
    }

    static void init_riscv_pci_drivers(const void *const dtb) {
        int pci_offset =
            fdt_node_offset_by_compatible(dtb, -1, "pci-host-ecam-generic");

        if (pci_offset < 0) {
            pci_offset =
                fdt_node_offset_by_compatible(dtb, -1, "pci-host-cam-generic");

            if (pci_offset < 0) {
                printk(LOGLEVEL_WARN, "dtb: \"pci\" node not found");
                return;
            }
        }

        struct string_view device_type = {};
        const bool get_compat_result =
            dtb_get_string_prop(dtb, pci_offset, "device_type", &device_type);

        if (!get_compat_result) {
            printk(LOGLEVEL_WARN,
                   "dtb: \"device_type\" property of pci node is missing");
            return;
        }

        if (sv_compare_c_str(device_type, "pci") != 0) {
            printk(LOGLEVEL_WARN,
                   "dtb: \"compatible\" property of pci node is not pci, got "
                   SV_FMT " instead\n",
                   SV_FMT_ARGS(device_type));
            return;
        }

        const fdt32_t *bus_range_array = NULL;
        uint32_t bus_range_length = 0;

        const bool get_bus_range_result =
            dtb_get_array_prop(dtb,
                               pci_offset,
                               "bus-range",
                               &bus_range_array,
                               &bus_range_length);

        if (!get_bus_range_result) {
            printk(LOGLEVEL_INFO,
                   "dtb: 'bus-range' property of pci node is missing or "
                   "malformed\n");
            return;
        }

        if (bus_range_length != 2) {
            printk(LOGLEVEL_INFO,
                   "dtb: 'bus-range' property of pci node is malformed\n");
            return;
        }

        const fdt32_t *domain_array = NULL;
        uint32_t domain_array_length = 0;

        const bool get_domain_result =
            dtb_get_array_prop(dtb,
                               pci_offset,
                               "linux,pci-domain",
                               &domain_array,
                               &domain_array_length);

        if (!get_domain_result) {
            printk(LOGLEVEL_INFO,
                   "dtb: domain property of pci node is missing or "
                   "malformed\n");
            return;
        }

        if (domain_array_length != 1) {
            printk(LOGLEVEL_INFO,
                   "dtb: domain property of pci node is malformed\n");
            return;
        }

        struct dtb_reg_entry base_addr_reg = {0};
        uint32_t entry_count = 1;

        const bool get_base_addr_reg_result =
            dtb_get_reg_entries(dtb,
                                pci_offset,
                                /*start_index=*/0,
                                &entry_count,
                                &base_addr_reg);

        if (!get_base_addr_reg_result) {
            printk(LOGLEVEL_INFO,
                   "dtb: base-addr reg of 'reg' property of pci node is "
                   "malformed\n");
            return;
        }

        const struct range bus_range =
            range_create(fdt32_to_cpu(bus_range_array[0]),
                         fdt32_to_cpu(bus_range_array[1]));

        pci_group_create_pcie(bus_range,
                              base_addr_reg.address,
                              /*segment=*/fdt32_to_cpu(domain_array[0]));
    }
#endif /* defined(__riscv) && defined(__LP64__) */

void dtb_init_early() {
    if (dtb_request.response == NULL) {
    #if defined(__riscv) && defined(__LP64__)
        panic("dtb: device tree not found");
    #else
        printk(LOGLEVEL_WARN, "dtb: device tree not found. exiting init");
        return;
    #endif /* defined(__riscv) && defined(__LP64__) */
    }

#if defined(__riscv) && defined(__LP64__)
    void *dtb = dtb_request.response->dtb_ptr;
    init_riscv_serial_drivers(dtb);
#endif /* defined(__riscv) && defined(__LP64__) */

    printk(LOGLEVEL_INFO, "dtb: finished initializing");
}

void dtb_init() {
    if (dtb_request.response == NULL) {
    #if defined(__riscv) && defined(__LP64__)
        panic("dtb: device tree not found");
    #else
        printk(LOGLEVEL_WARN, "dev: dtb not found\n");
        return;
    #endif /* defined(__riscv) && defined(__LP64__) */
    }

#if defined(__riscv) && defined(__LP64__)
    void *dtb = dtb_request.response->dtb_ptr;
    init_riscv_pci_drivers(dtb);
#endif /* defined(__riscv) && defined(__LP64__) */
}
