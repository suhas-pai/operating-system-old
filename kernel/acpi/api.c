/*
 * kernel/acpi/init.c
 * © suhas pai
 */

#include "acpi/mcfg.h"
#include "dev/printk.h"
#include "mm/mm_types.h"

#include "api.h"
#include "boot.h"
#include "fadt.h"
#include "madt.h"

static struct acpi_info info = {
    .madt = NULL,
    .fadt = NULL,
    .mcfg = NULL,
    .rsdp = NULL,
    .rsdt = NULL,

#if defined(__aarch64__)
    .msi_frame_list = ARRAY_INIT(sizeof(struct acpi_msi_frame)),
#endif /* defined(__aarch64__) */

    .iso_list = ARRAY_INIT(sizeof(struct apic_iso_info)),
    .nmi_lint = 0,

#if defined(__x86_64__)
    .using_x2apic = false
#endif /* defined(__x86_64__) */
};

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
    printk(LOGLEVEL_INFO,
           "acpi: found sdt \"" SV_FMT "\"\n",
           SV_FMT_ARGS(sv_create_nocheck(sdt->signature, 4)));

    if (memcmp(sdt->signature, "APIC", 4) == 0) {
        info.madt = (const struct acpi_madt *)sdt;
        return;
    }

    if (memcmp(sdt->signature, "FACP", 4) == 0) {
        info.fadt = (const struct acpi_fadt *)sdt;
        return;
    }

    if (memcmp(sdt->signature, "MCFG", 4) == 0) {
        info.mcfg = (const struct acpi_mcfg *)sdt;
    }
}

void acpi_init(void) {
    info.rsdp = boot_get_rsdp();
    if (info.rsdp == NULL) {
        return;
    }

    const uint64_t oem_id_length =
        strnlen(info.rsdp->oem_id, sizeof(info.rsdp->oem_id));
    const struct string_view oem_id =
        sv_create_nocheck(info.rsdp->oem_id, oem_id_length);

    printk(LOGLEVEL_INFO, "acpi: oem is \"" SV_FMT "\"\n", SV_FMT_ARGS(oem_id));
    if (has_xsdt()) {
        info.rsdt = phys_to_virt(info.rsdp->v2.xsdt_addr);
    } else {
        info.rsdt = phys_to_virt(info.rsdp->rsdt_addr);
    }

    printk(LOGLEVEL_INFO, "acpi: revision: %" PRIu8 "\n", info.rsdp->revision);
    printk(LOGLEVEL_INFO, "acpi: uses xsdt? %s\n", has_xsdt() ? "yes" : "no");
    printk(LOGLEVEL_INFO, "acpi: rsdt at %p\n", info.rsdt);

    acpi_recurse(acpi_init_each_sdt);
    if (get_acpi_info()->madt != NULL) {
        madt_init(get_acpi_info()->madt);
    }

    if (get_acpi_info()->fadt != NULL) {
        fadt_init(get_acpi_info()->fadt);
    }

    if (get_acpi_info()->mcfg != NULL) {
        mcfg_init(get_acpi_info()->mcfg);
    }
}

struct acpi_sdt *acpi_lookup_sdt(const char signature[static const 4]) {
    if (get_acpi_info()->rsdp == NULL) {
        return NULL;
    }

    if (has_xsdt()) {
        uint64_t *const data = (uint64_t *)(uint64_t)info.rsdt->ptrs;
        const uint32_t entry_count =
            (info.rsdt->sdt.length - sizeof(struct acpi_sdt)) /
            sizeof(uint64_t);

        for (uint32_t i = 0; i != entry_count; i++) {
            struct acpi_sdt *const sdt = phys_to_virt(data[i]);
            if (memcmp(sdt->signature, signature, 4) == 0) {
                return sdt;
            }
        }
    } else {
        uint32_t *const data = (uint32_t *)(uint64_t)info.rsdt->ptrs;
        const uint32_t entry_count =
            (info.rsdt->sdt.length - sizeof(struct acpi_sdt)) /
            sizeof(uint32_t);

        for (uint32_t i = 0; i != entry_count; i++) {
            struct acpi_sdt *const sdt = phys_to_virt(data[i]);
            if (memcmp(sdt->signature, signature, 4) == 0) {
                return sdt;
            }
        }
    }

    printk(LOGLEVEL_WARN,
           "acpi: failed to find entry with signature \"" SV_FMT "\"\n",
           SV_FMT_ARGS(sv_create_nocheck(signature, 4)));
    return NULL;
}

__optimize(3) const struct acpi_info *get_acpi_info() {
    return &info;
}

__optimize(3) struct acpi_info *get_acpi_info_mut() {
    return &info;
}