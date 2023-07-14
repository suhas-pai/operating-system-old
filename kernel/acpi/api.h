/*
 * kernel/acpi/init.h
 * © suhas pai
 */

#pragma once

#if defined(__x86_64__)
    #include "apic/structs.h"
#endif /* defined(__x86_64__) */

#include "acpi/structs.h"
#include "lib/adt/array.h"

#if defined(__x86_64__)
    struct lapic_info {
        uint8_t apic_id;
        uint8_t processor_id;

        bool enabled : 1;
        bool online_capable : 1;
    };

    struct ioapic_info {
        uint8_t id;
        uint8_t version;

        uint32_t gsi_base;
        uint8_t max_redirect_count;

        /* gsib = Global System Interrupt Base */
        volatile struct ioapic_registers *regs;
    };
#endif /* defined(__x86_64__) */

struct apic_iso_info {
    uint8_t bus_src;
    uint8_t irq_src;
    uint8_t gsi;
    uint16_t flags;
};

struct acpi_info {
    const struct acpi_madt *madt;
    const struct acpi_fadt *fadt;

    const struct acpi_rsdp *rsdp;
    const struct acpi_rsdt *rsdt;

#if defined(__x86_64__)
    volatile struct lapic_registers *lapic_regs;
#endif /* defined(__x86_64__) */

#if defined(__x86_64__)
    // Array of struct lapic_info
    struct array lapic_list;
    // Array of struct ioapic_info
    struct array ioapic_list;
#endif /* defined(__x86_64__) */
    struct array iso_list;

    uint8_t nmi_lint : 1;
#if defined(__x86_64__)
    bool using_x2apic : 1;
#endif /* defined(__x86_64__) */
};

void acpi_init();
struct acpi_sdt *acpi_lookup_sdt(const char signature[static 4]);

const struct acpi_info *get_acpi_info();
struct acpi_info *get_acpi_info_mut();