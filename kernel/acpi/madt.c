/*
 * kernel/acpi/madt.c
 * © suhas pai
 */

#if defined(__x86_64__)
    #include "apic/ioapic.h"
    #include "apic/init.h"
#endif /* defined(__x86_64__) */

#include "acpi/api.h"
#include "dev/printk.h"

#include "lib/align.h"

#include "mm/page.h"
#include "mm/pagemap.h"

void madt_init(const struct acpi_madt *const madt) {
    const struct acpi_madt_entry_header *iter = NULL;

#if defined(__x86_64__)
    uint64_t local_apic_base = madt->local_apic_base;
#endif /* defined(_-x86_64__) */

    uint32_t length = madt->sdt.length - sizeof(*madt);
    for (uint32_t offset = 0, index = 0;
         offset + sizeof(struct acpi_madt_entry_header) <= length;
         offset += iter->length, index++)
    {
        iter =
            (const struct acpi_madt_entry_header *)
                (madt->madt_entries + offset);

        switch (iter->kind) {
            case ACPI_MADT_ENTRY_KIND_CPU_LOCAL_APIC: {
                if (iter->length != sizeof(struct acpi_madt_entry_cpu_lapic)) {
                    printk(LOGLEVEL_INFO,
                           "apic: invalid local-apic madt entry at "
                           "index: %" PRIu32 "\n",
                           index);
                    continue;
                }

            #if defined(__x86_64__)
                const struct acpi_madt_entry_cpu_lapic *const hdr =
                    (const struct acpi_madt_entry_cpu_lapic *)iter;

                printk(LOGLEVEL_INFO,
                       "apic: found madt-entry cpu local-apic\n");

                printk(LOGLEVEL_INFO, "\tapic id: %" PRIu8 "\n", hdr->apic_id);
                printk(LOGLEVEL_INFO,
                       "\tprocessor id: %" PRIu8 "\n",
                       hdr->processor_id);

                const struct lapic_info lapic_info = {
                    .apic_id = hdr->apic_id,
                    .processor_id = hdr->processor_id,
                    .enabled =
                        (hdr->flags & ACPI_MADT_ENTRY_CPU_LAPIC_FLAG_ENABLED),
                    .online_capable =
                        (hdr->flags &
                         ACPI_MADT_ENTRY_CPU_LAPIC_FLAG_ONLINE_CAPABLE),
                };

                if (!lapic_info.enabled) {
                    if (lapic_info.online_capable) {
                        printk(LOGLEVEL_INFO,
                               "apic: cpu #%" PRIu32 " (with processor "
                               "id: %" PRIu8 " and local-apic id: %" PRIu8
                               ") has flag bit 0 disabled, but CAN be "
                               "enabled\n",
                               index,
                               hdr->processor_id,
                               hdr->apic_id);
                    } else {
                        printk(LOGLEVEL_INFO,
                               "apic: \tcpu #%" PRIu32 " (with processor "
                               "id: %" PRIu8 " and local-apic id: %" PRIu8
                               ") CANNOT be enabled\n",
                               index,
                               hdr->processor_id,
                               hdr->apic_id);
                    }
                } else {
                    printk(LOGLEVEL_INFO,
                           "apic: cpu #%" PRIu32 " (with processor "
                           "id: %" PRIu8 " and local-apic id: %" PRIu8
                           ") CAN be enabled\n",
                           index,
                           hdr->processor_id,
                           hdr->apic_id);
                }

                assert_msg(
                    array_append(&get_acpi_info_mut()->lapic_list, &lapic_info),
                    "Failed to add Local APIC info to array");
            #endif /* defined(__x86_64__) */

                break;
            }
            case ACPI_MADT_ENTRY_KIND_IO_APIC: {
                if (iter->length != sizeof(struct acpi_madt_entry_ioapic)) {
                    printk(LOGLEVEL_INFO,
                           "apic: invalid io-apic madt entry at "
                           "index: %" PRIu32 "\n",
                           index);
                    continue;
                }

            #if defined(__x86_64__)
                const struct acpi_madt_entry_ioapic *const hdr =
                    (const struct acpi_madt_entry_ioapic *)iter;

                printk(LOGLEVEL_INFO, "apic: found madt-entry io-apic\n");
                printk(LOGLEVEL_INFO, "\tapic id: %" PRIu8 "\n", hdr->apic_id);
                printk(LOGLEVEL_INFO, "\tbase: 0x%" PRIx32 "\n", hdr->base);
                printk(LOGLEVEL_INFO,
                       "\tglobal system interrupt base: 0x%" PRIx32 "\n",
                       hdr->gsib);

                assert_msg(has_align(hdr->base, PAGE_SIZE),
                           "io-apic base is not aligned on a page boundary");

                struct ioapic_info info = {
                    .id = hdr->apic_id,
                    .gsi_base = hdr->gsib,
                    .regs_mmio =
                        vmap_mmio(range_create(hdr->base, PAGE_SIZE),
                                  PROT_READ | PROT_WRITE)
                };

                const uint32_t id_reg =
                    ioapic_read(info.regs_mmio->base, IOAPIC_REG_ID);
                assert_msg(info.id == ioapic_id_reg_get_id(id_reg),
                           "io-apic ID in MADT doesn't match ID in MMIO");

                const uint32_t ioapic_version_reg =
                    ioapic_read(info.regs_mmio->base, IOAPIC_REG_VERSION);

                info.version =
                    ioapic_version_reg_get_version(ioapic_version_reg);
                info.max_redirect_count =
                    ioapic_version_reg_get_max_redirect_count(
                        ioapic_version_reg);

                printk(LOGLEVEL_INFO,
                       "ioapic: version: %" PRIu8 "\n",
                       info.version);
                printk(LOGLEVEL_INFO,
                       "ioapic: max redirect count: %" PRIu8 "\n",
                       info.max_redirect_count);

                assert_msg(
                    array_append(&get_acpi_info_mut()->ioapic_list, &info),
                    "Failed to add IO-APIC base to array");
            #else
                printk(LOGLEVEL_WARN,
                       "apic: found ioapic entry in madt. ignoring");
            #endif /* defined(__x86_64__) */
                break;
            }
            case ACPI_MADT_ENTRY_KIND_INT_SRC_OVERRIDE: {
                if (iter->length != sizeof(struct acpi_madt_entry_iso)) {
                    printk(LOGLEVEL_INFO,
                           "apic: invalid int-src override madt entry at "
                           "index: %" PRIu32,
                           index);
                    continue;
                }

                const struct acpi_madt_entry_iso *const hdr =
                    (const struct acpi_madt_entry_iso *)iter;

                printk(LOGLEVEL_INFO,
                       "apic: found madt-entry interrupt source override\n");
                printk(LOGLEVEL_INFO,
                       "\tbus source: %" PRIu8 "\n",
                       hdr->bus_source);
                printk(LOGLEVEL_INFO,
                       "\tirq source: %" PRIu8 "\n",
                       hdr->irq_source);

                printk(LOGLEVEL_INFO,
                       "\tglobal system interrupt: %" PRIu8 "\n",
                       hdr->gsi);

                printk(LOGLEVEL_INFO, "\tFlags: 0x%" PRIx16 "\n", hdr->flags);

                const struct apic_iso_info info = {
                    .bus_src = hdr->bus_source,
                    .irq_src = hdr->irq_source,
                    .gsi = hdr->gsi,
                    .flags = hdr->flags
                };

                assert_msg(array_append(&get_acpi_info_mut()->iso_list, &info),
                           "Failed to add APIC ISO-Info to array");
                break;
            }
            case ACPI_MADT_ENTRY_KIND_NON_MASKABLE_INT_SRC: {
                if (iter->length == sizeof(struct acpi_madt_entry_nmi_src)) {
                    printk(LOGLEVEL_INFO,
                           "apic: invalid nmi source madt entry at "
                           "index: %" PRIu32 "\n",
                           index);
                    continue;
                }

                const struct acpi_madt_entry_nmi_src *const hdr =
                    (const struct acpi_madt_entry_nmi_src *)iter;

                printk(LOGLEVEL_INFO,
                       "apic: found madt-entry non-maskable interrupt "
                       "source\n");

                printk(LOGLEVEL_INFO, "\tsource: %" PRIu8 "\n", hdr->source);
                printk(LOGLEVEL_INFO,
                       "\tglobal system interrupt: %" PRIu32 "\n",
                       hdr->gsi);

                printk(LOGLEVEL_INFO, "\tflags: 0x%" PRIu16 "\n", hdr->flags);
                break;
            }
            case ACPI_MADT_ENTRY_KIND_NON_MASKABLE_INT: {
                if (iter->length != sizeof(struct acpi_madt_entry_nmi)) {
                    printk(LOGLEVEL_INFO,
                           "apic: invalid nmi override madt entry at "
                           "index: %" PRIu32 "\n",
                           index);
                    continue;
                }

                const struct acpi_madt_entry_nmi *const hdr =
                    (const struct acpi_madt_entry_nmi *)iter;

                printk(LOGLEVEL_INFO,
                       "apic: found madt-entry non-maskable interrupt\n");
                printk(LOGLEVEL_INFO,
                       "\tprocessor: %" PRIu8 "\n",
                       hdr->processor);

                printk(LOGLEVEL_INFO, "\tflags: %" PRIu16 "\n", hdr->flags);
                printk(LOGLEVEL_INFO, "\tlint: %" PRIu8 "\n", hdr->lint);

                get_acpi_info_mut()->nmi_lint = hdr->lint;
                break;
            }
            case ACPI_MADT_ENTRY_KIND_LOCAL_APIC_ADDR_OVERRIDE: {
                if (iter->length !=
                        sizeof(struct acpi_madt_entry_lapic_addr_override))
                {
                    printk(LOGLEVEL_INFO,
                           "apic: invalid lapic addr override madt entry at "
                           "index: %" PRIu32 "\n",
                           index);
                    continue;
                }

            #if defined(__x86_64__)
                const struct acpi_madt_entry_lapic_addr_override *const hdr =
                    (const struct acpi_madt_entry_lapic_addr_override *)iter;

                printk(LOGLEVEL_INFO,
                       "apic: found madt-entry local-apic address override\n");

                printk(LOGLEVEL_INFO, "\tbase: 0x%" PRIx64 "\n", hdr->base);
                local_apic_base = hdr->base;
            #endif /* defined(__x86_64__) */

                break;
            }
            case ACPI_MADT_ENTRY_KIND_CPU_LOCAL_X2APIC: {
                if (iter->length !=
                        sizeof(struct acpi_madt_entry_cpu_local_x2apic))
                {
                    printk(LOGLEVEL_INFO,
                           "apic: invalid local x2apic madt entry at "
                           "index: %" PRIu32 "\n",
                           index);
                    continue;
                }

                const struct acpi_madt_entry_cpu_local_x2apic *const hdr =
                    (const struct acpi_madt_entry_cpu_local_x2apic *)iter;

                printk(LOGLEVEL_INFO, "apic: found madt-entry local-x2apic\n");
                printk(LOGLEVEL_INFO, "\tacpi id: %" PRIu32 "\n", hdr->acpi_id);
                printk(LOGLEVEL_INFO,
                       "\tx2acpi id: %" PRIu32 "\n",
                       hdr->x2apic_id);

                printk(LOGLEVEL_INFO, "\tflags: 0x%" PRIx32 "\n", hdr->flags);
                break;
            }
            default:
                printk(LOGLEVEL_INFO,
                       "apic: found invalid madt-entry: %" PRIu32 "\n",
                       iter->kind);
                continue;
        }
    }

#if defined(__x86_64__)
    assert_msg(local_apic_base != 0, "Failed to find local-apic registers");
    get_acpi_info_mut()->lapic_regs =
        vmap_mmio(range_create(local_apic_base, PAGE_SIZE),
                    PROT_READ | PROT_WRITE);

    apic_init();
#endif /* defined(__x86_64__) */
}