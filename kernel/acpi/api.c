/*
 * kernel/acpi/init.c
 * © suhas pai
 */

#include "dev/printk.h"
#include "lib/string.h"
#include "mm/page.h"

#include "api.h"
#include "fadt.h"
#include "limine.h"
#include "madt.h"

static volatile struct limine_rsdp_request rsdp_request = {
    .id = LIMINE_RSDP_REQUEST,
    .revision = 0
};

static struct acpi_info info = {};

static inline bool has_xsdt() {
    return (info.rsdp->revision >= 2 && info.rsdp->v2.xsdt_addr != 0);
}

static inline void acpi_recurse(void (*callback)(const struct acpi_sdt *)) {
    if (has_xsdt()) {
        uint64_t *const data = (uint64_t *)(uint64_t)info.rsdt->ptrs;
        const uint32_t entry_count =
            (info.rsdt->sdt.length - sizeof(struct acpi_sdt)) /
            sizeof(uint64_t);

        for (uint32_t i = 0; i != entry_count; i++) {
            struct acpi_sdt *const sdt = phys_to_virt(data[i]);
            callback(sdt);
        }
    } else {
        uint32_t *const data = (uint32_t *)(uint64_t)info.rsdt->ptrs;
        const uint32_t entry_count =
            (info.rsdt->sdt.length - sizeof(struct acpi_sdt)) /
            sizeof(uint32_t);

        for (uint32_t i = 0; i != entry_count; i++) {
            struct acpi_sdt *const sdt = phys_to_virt(data[i]);
            callback(sdt);
        }
    }
}

static inline void acpi_init_each_sdt(const struct acpi_sdt *const sdt) {
    if (memcmp(sdt->signature, "APIC", 4) == 0) {
        info.madt = (const struct acpi_madt *)sdt;
        madt_init(info.madt);

        return;
    }

    if (memcmp(sdt->signature, "FACP", 4) == 0) {
        info.fadt = (const struct acpi_fadt *)sdt;
        fadt_init(info.fadt);

        return;
    }
}

void acpi_init(void) {
    struct limine_rsdp_response *const response = rsdp_request.response;
    if (response == NULL || response->address == NULL) {
        panic("ACPI is not supported on this machine");
    }

    info.rsdp = response->address;
    if (has_xsdt()) {
        info.rsdt = phys_to_virt(info.rsdp->v2.xsdt_addr);
    } else {
        info.rsdt = phys_to_virt(info.rsdp->rsdt_addr);
    }

    printk(LOGLEVEL_INFO, "acpi: Revision: %" PRIu8 "\n", info.rsdp->revision);
    printk(LOGLEVEL_INFO, "acpi: Uses XSDT? %s\n", has_xsdt() ? "yes" : "no");
    printk(LOGLEVEL_INFO, "acpi: RSDT at %p\n", info.rsdt);

    acpi_recurse(acpi_init_each_sdt);
}

struct acpi_sdt *acpi_lookup_sdt(const char signature[static const 4]) {
    if (has_xsdt()) {
        uint64_t *const data = (uint64_t *)(uint64_t)info.rsdt->ptrs;
        const uint32_t entry_count =
            (info.rsdt->sdt.length - sizeof(struct acpi_sdt)) / sizeof(uint64_t);

        for (uint32_t i = 0; i != entry_count; i++) {
            struct acpi_sdt *const sdt = phys_to_virt(data[i]);
            if (memcmp(sdt->signature, signature, 4) == 0) {
                return sdt;
            }
        }
    } else {
        uint32_t *const data = (uint32_t *)(uint64_t)info.rsdt->ptrs;
        const uint32_t entry_count =
            (info.rsdt->sdt.length - sizeof(struct acpi_sdt)) / sizeof(uint32_t);

        for (uint32_t i = 0; i != entry_count; i++) {
            struct acpi_sdt *const sdt = phys_to_virt(data[i]);
            if (memcmp(sdt->signature, signature, 4) == 0) {
                return sdt;
            }
        }
    }

    printk(LOGLEVEL_DEBUG,
           "acpi: Failed to find entry with signature \"" SV_FMT "\"\n",
           SV_FMT_ARGS(sv_create_nocheck(signature, 4)));

    return NULL;
}

const struct acpi_info *get_acpi_info() {
    return &info;
}

struct acpi_info *get_acpi_info_mut() {
    return &info;
}