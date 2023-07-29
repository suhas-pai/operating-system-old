/*
 * kernel/acpi/init.c
 * © suhas pai
 */

#include "acpi/mcfg.h"
#if defined(__riscv) && defined (__LP64__)
    #include "acpi/rhct.h"
#endif /* defined(__riscv) && defined (__LP64__) */

#include "dev/printk.h"
#include "mm/page.h"

#include "api.h"
#include "fadt.h"
#include "limine.h"
#include "madt.h"

static volatile struct limine_rsdp_request rsdp_request = {
    .id = LIMINE_RSDP_REQUEST,
    .revision = 0
};

static struct acpi_info info = {0};
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

#if defined(__riscv) && defined(__LP64__)
    if (memcmp(sdt->signature, "RHCT", 4) == 0) {
        info.rhct = (const struct acpi_rhct *)sdt;
    }
#endif /* defined(__riscv) && defined(__LP64__) */
}

void acpi_init(void) {
    struct limine_rsdp_response *const response = rsdp_request.response;
    if (response == NULL || response->address == NULL) {
    #if !(defined(__riscv) && defined(__LP64__))
        panic("acpi: not found\n");
    #else
        printk(LOGLEVEL_WARN, "acpi: does not exist. exiting init\n");
        return;
    #endif /* !(defined(__riscv) && defined(__LP64__)) */
    }

#if defined(__x86_64__)
    array_init(&get_acpi_info_mut()->lapic_list, sizeof(struct lapic_info));
    array_init(&get_acpi_info_mut()->ioapic_list, sizeof(struct ioapic_info));
#elif defined(__aarch64__)
    array_init(&get_acpi_info_mut()->msi_frame_list,
               sizeof(struct acpi_msi_frame));
#endif /* defined(__x86_64__) */

    array_init(&get_acpi_info_mut()->iso_list, sizeof(struct apic_iso_info));
    info.rsdp = response->address;

    if (has_xsdt()) {
        info.rsdt = phys_to_virt(info.rsdp->v2.xsdt_addr);
    } else {
        info.rsdt = phys_to_virt(info.rsdp->rsdt_addr);
    }

    printk(LOGLEVEL_INFO, "acpi: revision: %" PRIu8 "\n", info.rsdp->revision);
    printk(LOGLEVEL_INFO, "acpi: uses xsdt? %s\n", has_xsdt() ? "yes" : "no");
    printk(LOGLEVEL_INFO, "acpi: rsdt at %p\n", info.rsdt);

    acpi_recurse(acpi_init_each_sdt);
#if defined(__riscv) && defined(__LP64__)
    if (get_acpi_info()->rhct != NULL) {
        acpi_rhct_init(get_acpi_info()->rhct);
    }
#endif /* defined(__riscv) && defined(__LP64__) */

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

const struct acpi_info *get_acpi_info() {
    return &info;
}

struct acpi_info *get_acpi_info_mut() {
    return &info;
}