/*
 * kernel/acpi/madt.c
 * © suhas pai
 */

#include "acpi/structs.h"
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
                           "madt: invalid local-apic entry at "
                           "index: %" PRIu32 "\n",
                           index);
                    continue;
                }

            #if defined(__x86_64__)
                const struct acpi_madt_entry_cpu_lapic *const hdr =
                    (const struct acpi_madt_entry_cpu_lapic *)iter;

                printk(LOGLEVEL_INFO,
                       "madt: found madt-entry cpu local-apic\n"
                       "\tapic id: %" PRIu8 "\n"
                       "\tprocessor id: %" PRIu8 "\n",
                       hdr->apic_id,
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
                               "madt: cpu #%" PRIu32 " (with processor "
                               "id: %" PRIu8 " and local-apic id: %" PRIu8
                               ") has flag bit 0 disabled, but CAN be "
                               "enabled\n",
                               index,
                               hdr->processor_id,
                               hdr->apic_id);
                    } else {
                        printk(LOGLEVEL_INFO,
                               "madt: \tcpu #%" PRIu32 " (with processor "
                               "id: %" PRIu8 " and local-apic id: %" PRIu8
                               ") CANNOT be enabled\n",
                               index,
                               hdr->processor_id,
                               hdr->apic_id);
                    }
                } else {
                    printk(LOGLEVEL_INFO,
                           "madt: cpu #%" PRIu32 " (with processor "
                           "id: %" PRIu8 " and local-apic id: %" PRIu8
                           ") CAN be enabled\n",
                           index,
                           hdr->processor_id,
                           hdr->apic_id);
                }

                assert_msg(
                    array_append(&get_acpi_info_mut()->lapic_list, &lapic_info),
                    "Failed to add Local APIC info to array");
            #else
                printk(LOGLEVEL_WARN,
                       "madt: found local-apic entry. ignoring");
            #endif /* defined(__x86_64__) */

                break;
            }
            case ACPI_MADT_ENTRY_KIND_IO_APIC: {
                if (iter->length != sizeof(struct acpi_madt_entry_ioapic)) {
                    printk(LOGLEVEL_INFO,
                           "madt: invalid io-apic entry at "
                           "index: %" PRIu32 "\n",
                           index);
                    continue;
                }

            #if defined(__x86_64__)
                const struct acpi_madt_entry_ioapic *const hdr =
                    (const struct acpi_madt_entry_ioapic *)iter;

                printk(LOGLEVEL_INFO,
                       "madt: found entry io-apic\n"
                       "\tapic id: %" PRIu8 "\n"
                       "\tbase: 0x%" PRIx32 "\n"
                       "\tglobal system interrupt base: 0x%" PRIx32 "\n",
                       hdr->apic_id,
                       hdr->base,
                       hdr->gsib);

                assert_msg(has_align(hdr->base, PAGE_SIZE),
                           "io-apic base is not aligned on a page boundary");

                struct ioapic_info info = {
                    .id = hdr->apic_id,
                    .gsi_base = hdr->gsib,
                    .regs_mmio =
                        vmap_mmio(range_create(hdr->base, PAGE_SIZE),
                                  PROT_READ | PROT_WRITE,
                                  /*flags=*/0)
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
                       "ioapic: version: %" PRIu8 "\n"
                       "ioapic: max redirect count: %" PRIu8 "\n",
                       info.version,
                       info.max_redirect_count);

                assert_msg(
                    array_append(&get_acpi_info_mut()->ioapic_list, &info),
                    "Failed to add IO-APIC base to array");
            #else
                printk(LOGLEVEL_WARN,
                       "madt: found ioapic entry. ignoring");
            #endif /* defined(__x86_64__) */
                break;
            }
            case ACPI_MADT_ENTRY_KIND_INT_SRC_OVERRIDE: {
                if (iter->length != sizeof(struct acpi_madt_entry_iso)) {
                    printk(LOGLEVEL_INFO,
                           "madt: invalid int-src override entry at "
                           "index: %" PRIu32,
                           index);
                    continue;
                }

                const struct acpi_madt_entry_iso *const hdr =
                    (const struct acpi_madt_entry_iso *)iter;

                printk(LOGLEVEL_INFO,
                       "madt: found entry interrupt source override\n"
                       "\tbus source: %" PRIu8 "\n"
                       "\tirq source: %" PRIu8 "\n"
                       "\tglobal system interrupt: %" PRIu8 "\n"
                       "\tflags: 0x%" PRIx16 "\n"
                       "\t\tactive-low: %s\n"
                       "\t\tlevel-triggered: %s\n",
                       hdr->bus_source,
                       hdr->irq_source,
                       hdr->gsi,
                       hdr->flags,
                       (hdr->flags & __ACPI_MADT_ENTRY_ISO_ACTIVE_LOW) != 0 ?
                        "yes" : "no",
                       (hdr->flags & __ACPI_MADT_ENTRY_ISO_LEVEL_TRIGGER) != 0 ?
                        "yes" : "no");

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
                           "madt: invalid nmi source entry at "
                           "index: %" PRIu32 "\n",
                           index);
                    continue;
                }

                const struct acpi_madt_entry_nmi_src *const hdr =
                    (const struct acpi_madt_entry_nmi_src *)iter;

                printk(LOGLEVEL_INFO,
                       "madt: found entry non-maskable interrupt source\n"
                       "\tsource: %" PRIu8 "\n"
                       "\tglobal system interrupt: %" PRIu32 "\n"
                       "\tflags: 0x%" PRIx16 "\n"
                       "\t\tactive-low: %s\n"
                       "\t\tlevel-triggered: %s\n",
                       hdr->source,
                       hdr->gsi,
                       hdr->flags,
                       (hdr->flags & __ACPI_MADT_ENTRY_ISO_ACTIVE_LOW) != 0 ?
                        "yes" : "no",
                       (hdr->flags & __ACPI_MADT_ENTRY_ISO_LEVEL_TRIGGER) != 0 ?
                        "yes" : "no");
                break;
            }
            case ACPI_MADT_ENTRY_KIND_NON_MASKABLE_INT: {
                if (iter->length != sizeof(struct acpi_madt_entry_nmi)) {
                    printk(LOGLEVEL_INFO,
                           "madt: invalid nmi override entry at "
                           "index: %" PRIu32 "\n",
                           index);
                    continue;
                }

                const struct acpi_madt_entry_nmi *const hdr =
                    (const struct acpi_madt_entry_nmi *)iter;

                printk(LOGLEVEL_INFO,
                       "madt: found entry non-maskable interrupt\n"
                       "\tprocessor: %" PRIu8 "\n"
                       "\tflags: %" PRIu16 "\n"
                       "\tlint: %" PRIu8 "\n",
                       hdr->processor,
                       hdr->flags,
                       hdr->lint);

                get_acpi_info_mut()->nmi_lint = hdr->lint;
                break;
            }
            case ACPI_MADT_ENTRY_KIND_LOCAL_APIC_ADDR_OVERRIDE: {
                if (iter->length !=
                        sizeof(struct acpi_madt_entry_lapic_addr_override))
                {
                    printk(LOGLEVEL_INFO,
                           "madt: invalid lapic addr override entry at "
                           "index: %" PRIu32 "\n",
                           index);
                    continue;
                }

            #if defined(__x86_64__)
                const struct acpi_madt_entry_lapic_addr_override *const hdr =
                    (const struct acpi_madt_entry_lapic_addr_override *)iter;

                printk(LOGLEVEL_INFO,
                       "madt: found entry local-apic address override\n"
                       "\tbase: 0x%" PRIx64 "\n",
                       hdr->base);

                local_apic_base = hdr->base;
            #else
                printk(LOGLEVEL_WARN,
                       "madt: found local-apic addr override entry. ignoring");
            #endif /* defined(__x86_64__) */

                break;
            }
            case ACPI_MADT_ENTRY_KIND_CPU_LOCAL_X2APIC: {
                if (iter->length !=
                        sizeof(struct acpi_madt_entry_cpu_local_x2apic))
                {
                    printk(LOGLEVEL_INFO,
                           "madt: invalid local x2apic entry at "
                           "index: %" PRIu32 "\n",
                           index);
                    continue;
                }

            #if defined(__x86_64__)
                const struct acpi_madt_entry_cpu_local_x2apic *const hdr =
                    (const struct acpi_madt_entry_cpu_local_x2apic *)iter;

                printk(LOGLEVEL_INFO,
                       "madt: found entry local-x2apic\n"
                       "\tacpi id: %" PRIu32 "\n"
                       "\tx2acpi id: %" PRIu32 "\n"
                       "\tflags: 0x%" PRIx32 "\n",
                       hdr->acpi_uid,
                       hdr->x2apic_id,
                       hdr->flags);
            #else
                printk(LOGLEVEL_WARN,
                       "madt: found x2apic entry. ignoring");
            #endif /* defined(__x86_64__) */

                break;
            }
            case ACPI_MADT_ENTRY_KIND_CPU_LOCAL_X2APIC_NMI: {
                if (iter->length !=
                        sizeof(struct acpi_madt_entry_cpu_local_x2apic_nmi))
                {
                    printk(LOGLEVEL_INFO,
                           "madt: invalid local x2apic nmi entry at "
                           "index: %" PRIu32 "\n",
                           index);
                    continue;
                }

            #if defined(__x86_64__)
                const struct acpi_madt_entry_cpu_local_x2apic_nmi *const hdr =
                    (const struct acpi_madt_entry_cpu_local_x2apic_nmi *)iter;

                printk(LOGLEVEL_INFO,
                       "madt: found entry local-x2apic nmi\n"
                       "\tacpi uid: %" PRIu32 "\n"
                       "\tflags: 0x%" PRIx32 "\n"
                       "\tx2acpi lint: %" PRIu32 "\n",
                       hdr->acpi_uid,
                       hdr->flags,
                       hdr->local_x2apic_lint);
            #else
                printk(LOGLEVEL_WARN,
                       "madt: found x2apic nmi entry. ignoring");
            #endif /* defined(__x86_64__) */

                break;
            }
            case ACPI_MADT_ENTRY_KIND_GIC_CPU_INTERFACE: {
                if (iter->length !=
                        sizeof(struct acpi_madt_entry_gic_cpu_interface))
                {
                    printk(LOGLEVEL_INFO,
                           "madt: invalid gic cpu-interface entry at "
                           "index: %" PRIu32 "\n",
                           index);
                    continue;
                }
            #if defined(__aarch64__)
                const struct acpi_madt_entry_gic_cpu_interface *const cpu =
                    (const struct acpi_madt_entry_gic_cpu_interface *)iter;

                printk(LOGLEVEL_DEBUG,
                       "madt: found gic cpu-interface:\n"
                       "\tinterface number: %" PRIu32 "\n"
                       "\tacpi processor id: %" PRIu32 "\n"
                       "\tflags: 0x%" PRIx32 "\n"
                       "\t\tcpu enabled: %s\n"
                       "\t\tperf interrupt edge-triggered: %s\n"
                       "\t\tvgic maintenance intr edge-triggered: %s\n"
                       "\tparking protocol version: %" PRIu32 "\n"
                       "\tperformance interrupt gsiv: %" PRIu32 "\n"
                       "\tparked address: 0x%" PRIx64 "\n"
                       "\tphys base address: 0x%" PRIx64 "\n"
                       "\tgic virt cpu reg address: 0x%" PRIx64 "\n"
                       "\tgic virt ctrl block address: 0x%" PRIx64 "\n"
                       "\tvgic maintenance interrupt: %" PRIu32 "\n"
                       "\tgicr phys base address: 0x%" PRIx64 "\n"
                       "\tmpidr: 0x%" PRIx64 "\n"
                       "\tprocessor power efficiency class: %" PRIu8 "\n"
                       "\tspe overflow interrupt: %" PRIu16 "\n",
                       cpu->cpu_interface_number,
                       cpu->acpi_processor_id,
                       cpu->flags,
                       (cpu->flags & ACPI_MADT_ENTRY_GIC_CPU_ENABLED) != 0 ?
                        "yes" : "no",
                       (cpu->flags &
                        ACPI_MADT_ENTRY_GIC_CPU_PERF_INTR_EDGE_TRIGGER) != 0 ?
                        "yes" : "no",
                       (cpu->flags &
                        ACPI_MADT_ENTRY_GIC_CPU_VGIC_INTR_EDGE_TRIGGER) != 0 ?
                        "yes" : "no",
                       cpu->parking_protocol_version,
                       cpu->perf_interrupt_gsiv,
                       cpu->parked_address,
                       cpu->phys_base_address,
                       cpu->gic_virt_cpu_reg_address,
                       cpu->gic_virt_ctrl_block_reg_address,
                       cpu->vgic_maintenance_interrupt,
                       cpu->gicr_phys_base_address,
                       cpu->mpidr,
                       cpu->processor_power_efficiency_class,
                       cpu->spe_overflow_interrupt);
            #else
                printk(LOGLEVEL_WARN,
                       "madt: found gic cpu-interface entry. ignoring");
            #endif /* defined(__aarch64__) */

                break;
            }
            case ACPI_MADT_ENTRY_KIND_GIC_DISTRIBUTOR: {
                if (iter->length !=
                        sizeof(struct acpi_madt_entry_gic_distributor))
                {
                    printk(LOGLEVEL_INFO,
                           "madt: invalid gic distributor entry at "
                           "index: %" PRIu32 "\n",
                           index);
                    continue;
                }
            #if defined(__aarch64__)
                const struct acpi_madt_entry_gic_distributor *const dist =
                    (const struct acpi_madt_entry_gic_distributor *)iter;

                printk(LOGLEVEL_DEBUG,
                       "madt: found gic distributor\n"
                       "\tgic hardware id: %" PRIu32 "\n"
                       "\tphys base address: 0x%" PRIx64 "\n"
                       "\tsystem vector base: %" PRIu32 "\n"
                       "\tgic version: %" PRIu8 "\n",
                       dist->gic_hardware_id,
                       dist->phys_base_address,
                       dist->sys_vector_base,
                       dist->gic_version);
            #else
                printk(LOGLEVEL_WARN,
                       "madt: found gic distributor entry. ignoring");
            #endif /* defined(__aarch64__) */

                break;
            }
            case ACPI_MADT_ENTRY_KIND_GIC_MSI_FRAME: {
                if (iter->length !=
                        sizeof(struct acpi_madt_entry_gic_msi_frame))
                {
                    printk(LOGLEVEL_INFO,
                           "madt: invalid gic msi-frame entry at "
                           "index: %" PRIu32 "\n",
                           index);
                    continue;
                }
            #if defined(__aarch64__)
                const struct acpi_madt_entry_gic_msi_frame *const frame =
                    (const struct acpi_madt_entry_gic_msi_frame *)iter;

                printk(LOGLEVEL_DEBUG,
                       "madt: found msi-frame\n"
                       "\tmsi frame id: %" PRIu32 "\n"
                       "\tphys base address: 0x%" PRIx64 "\n"
                       "\tflags: 0x%" PRIx8 "\n"
                       "\t\toverride msi_typer: %s\n"
                       "\tspi count: %" PRIu16 "\n"
                       "\tspi base: %" PRIu16 "\n",
                       frame->msi_frame_id,
                       frame->phys_base_address,
                       frame->flags,
                       (frame->flags &
                        __ACPI_MADT_GICMSI_FRAME_OVERR_MSI_TYPERR) != 0 ?
                            "yes" : "no",
                       frame->spi_count,
                       frame->spi_base);
            #else
                printk(LOGLEVEL_WARN,
                       "madt: found gic distributor entry. ignoring");
            #endif /* defined(__aarch64__) */

                break;
            }
            default:
                printk(LOGLEVEL_INFO,
                       "madt: found invalid entry: %" PRIu32 "\n",
                       iter->kind);
                continue;
        }
    }

#if defined(__x86_64__)
    assert_msg(local_apic_base != 0, "Failed to find local-apic registers");
    get_acpi_info_mut()->lapic_regs =
        vmap_mmio(range_create(local_apic_base, PAGE_SIZE),
                  PROT_READ | PROT_WRITE,
                  /*flags=*/0);

    apic_init();
#endif /* defined(__x86_64__) */
}