/*
 * kernel/acpi/madt.c
 * © suhas pai
 */

#include "dev/printk.h"

#include "lib/align.h"
#include "mm/page.h"

#include "madt.h"

void madt_init(const struct acpi_madt *const madt) {
    const struct acpi_madt_entry_header *iter = NULL;

    uint64_t local_apic_base = madt->local_apic_base;
    uint32_t lapic_index = 0;
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
                           "acpi: Invalid local-apic madt entry at "
                           "index: %" PRIu32 "\n",
                           index);
                    goto done;
                }

                const struct acpi_madt_entry_cpu_lapic *const hdr =
                    (const struct acpi_madt_entry_cpu_lapic *)iter;

                printk(LOGLEVEL_INFO,
                       "acpi: Found madt-entry cpu local-apic\n");

                printk(LOGLEVEL_INFO,
                       "\tAPIC ID: %" PRIu8 "\n",
                       hdr->apic_id);

                printk(LOGLEVEL_INFO,
                       "\tProcessor ID: %" PRIu8 "\n",
                       hdr->processor_id);

                #if 0
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
                               "acpi: \tCPU #%" PRIu32 " (with Processor "
                               "ID: %" PRIu8 " and local-apic id: %" PRIu8
                               " has flag bit 0 disabled, but CAN be enabled",
                               index,
                               hdr->processor_id,
                               hdr->apic_id);
                    } else {
                        printk(LOGLEVEL_INFO,
                               "acpi: \tCPU #%" PRIu32 " (with Processor "
                               "ID: %" PRIu8 " and local-apic id: %" PRIu8
                               " CANNOT be enabled",
                               index,
                               hdr->processor_id,
                               hdr->apic_id);
                    }
                } else {
                    printk(LOGLEVEL_INFO,
                           "acpi: \tCPU #%" PRIu32 " (with Processor "
                           "ID: %" PRIu8 " and local-apic id: %" PRIu8
                           " CAN be enabled",
                           index,
                           hdr->processor_id,
                           hdr->apic_id);
                }

                get_apic_info_mut()->lapic_list[lapic_i] = lapic_info;
                #endif

                lapic_index++;
                break;
            }
            case ACPI_MADT_ENTRY_KIND_IO_APIC: {
                if (iter->length != sizeof(struct acpi_madt_entry_ioapic)) {
                    printk(LOGLEVEL_INFO,
                           "acpi: Invalid io-apic madt entry at "
                           "index: %" PRIu32 "\n",
                           index);
                    goto done;
                }

                const struct acpi_madt_entry_ioapic *const hdr =
                    (const struct acpi_madt_entry_ioapic *)iter;

                printk(LOGLEVEL_INFO, "acpi: Found madt-entry io-apic\n");
                printk(LOGLEVEL_INFO,
                       "\tAPIC ID: %" PRIu8 "\n",
                       hdr->apic_id);
                printk(LOGLEVEL_INFO,
                       "\tBase: 0x%" PRIx32 "\n",
                       hdr->base);
                printk(LOGLEVEL_INFO,
                       "\tGlobal System Interrupt Base: 0x%" PRIx32 "\n",
                       hdr->gsib);

                assert_msg(has_align(hdr->base, PAGE_SIZE),
                           "IO-APIC base is not aligned on a page boundary");

                #if 0
                const uint64_t ioapic_base = phys_to_virt(hdr->base);
                const bool map_ioapic_base_result =
                    vmm_map_mmio_page(/*phys_addr=*/hdr->base,
                                      /*virt_addr=*/ioapic_base,
                                      VMM_PAGE_SIZE_ARCH_DEFAULT,
                                      /*allow_remap=*/true);

                assert_msg(map_ioapic_base_result,
                           "Failed to map IO-APIC base");

                struct ioapic_info info = {
                    .id = hdr->apic_id,
                    .gsi_base = hdr->gsib,
                    .regs = (volatile struct ioapic_registers *)ioapic_base
                };

                const uint32_t id_reg = ioapic_read(info.regs, IOAPIC_REG_ID);
                assert_msg(info.id == ioapic_id_reg_get_id(id_reg),
                           "IO-APIC ID in MADT doesn't match ID in MMIO");

                const uint32_t ioapic_version_reg =
                    ioapic_read(info.regs, IOAPIC_REG_VERSION);

                info.version =
                    ioapic_version_reg_get_version(ioapic_version_reg);
                info.max_redirect_count =
                    ioapic_version_reg_get_max_redirect_count(
                        ioapic_version_reg);

                const bool add_base_result =
                    array_add_item(&get_apic_info_mut()->ioapic_list,
                                   sizeof(info),
                                   &info);

                assert_msg(add_base_result,
                           "Failed to add IO-APIC base to array");
                #endif
                break;
            }
            case ACPI_MADT_ENTRY_KIND_INT_SRC_OVERRIDE: {
                if (iter->length != sizeof(struct acpi_madt_entry_iso)) {
                    printk(LOGLEVEL_INFO,
                           "acpi: Invalid int-src override madt entry at "
                           "index: %" PRIu32,
                           index);
                    goto done;
                }

                const struct acpi_madt_entry_iso *const hdr =
                    (const struct acpi_madt_entry_iso *)iter;

                printk(LOGLEVEL_INFO,
                       "acpi: Found madt-entry interrupt source override\n");
                printk(LOGLEVEL_INFO,
                       "\tBus Source: %" PRIu8 "\n",
                       hdr->bus_source);
                printk(LOGLEVEL_INFO,
                       "\tIrq Source: %" PRIu8 "\n",
                       hdr->irq_source);

                printk(LOGLEVEL_INFO,
                       "\tGlobal System Interrupt: %" PRIu8 "\n",
                       hdr->gsi);

                printk(LOGLEVEL_INFO, "\tFlags: 0x%" PRIx16 "\n", hdr->flags);

                #if 0
                const struct apic_iso_info info = {
                    .bus_src = hdr->bus_source,
                    .irq_src = hdr->irq_source,
                    .gsi = hdr->gsi,
                    .flags = hdr->flags
                };

                const bool add_result =
                    array_add_item(&get_apic_info_mut()->iso_list,
                                   sizeof(info),
                                   &info);

                assert_msg(add_result, "Failed to add APIC ISO-Info to array");
                #endif
                break;
            }
            case ACPI_MADT_ENTRY_KIND_NON_MASKABLE_INT_SRC: {
                if (iter->length == sizeof(struct acpi_madt_entry_nmi_src)) {
                    printk(LOGLEVEL_INFO,
                           "acpi: Invalid nmi source madt entry at "
                           "index: %" PRIu32 "\n",
                           index);
                    goto done;
                }

                const struct acpi_madt_entry_nmi_src *const hdr =
                    (const struct acpi_madt_entry_nmi_src *)iter;

                printk(LOGLEVEL_INFO,
                       "acpi: Found madt-entry non-maskable interrupt "
                       "source\n");

                printk(LOGLEVEL_INFO, "\tSource: %" PRIu8 "\n", hdr->source);
                printk(LOGLEVEL_INFO,
                       "\tGlobal System Interrupt: %" PRIu32 "\n",
                       hdr->gsi);

                printk(LOGLEVEL_INFO, "\tFlags: 0x%" PRIu16 "\n", hdr->flags);
                break;
            }
            case ACPI_MADT_ENTRY_KIND_NON_MASKABLE_INT: {
                if (iter->length != sizeof(struct acpi_madt_entry_nmi)) {
                    printk(LOGLEVEL_INFO,
                           "acpi: Invalid nmi override madt entry at "
                           "index: %" PRIu32 "\n",
                           index);
                    goto done;
                }

                const struct acpi_madt_entry_nmi *const hdr =
                    (const struct acpi_madt_entry_nmi *)iter;

                printk(LOGLEVEL_INFO,
                       "acpi: Found madt-entry non-maskable interrupt\n");
                printk(LOGLEVEL_INFO,
                       "\tProcessor: %" PRIu8 "\n",
                       hdr->processor);
                printk(LOGLEVEL_INFO,
                       "\tFlags: %" PRIu16 "\n",
                       hdr->flags);
                printk(LOGLEVEL_INFO,
                       "\tLint: %" PRIu8 "\n",
                       hdr->lint);

                //get_apic_info_mut()->nmi_lint = hdr->lint;
                break;
            }
            case ACPI_MADT_ENTRY_KIND_LOCAL_APIC_ADDR_OVERRIDE: {
                if (iter->length !=
                        sizeof(struct acpi_madt_entry_lapic_addr_override))
                {
                    printk(LOGLEVEL_INFO,
                           "acpi: Invalid lapic addr override madt entry at "
                           "index: %" PRIu32 "\n",
                           index);
                    goto done;
                }

                const struct acpi_madt_entry_lapic_addr_override *const hdr =
                    (const struct acpi_madt_entry_lapic_addr_override *)iter;

                printk(LOGLEVEL_INFO,
                       "acpi: Found madt-entry local-apic address override\n");

                printk(LOGLEVEL_INFO, "\tBase: 0x%" PRIx64 "\n", hdr->base);
                local_apic_base = hdr->base;

                break;
            }
            case ACPI_MADT_ENTRY_KIND_CPU_LOCAL_X2APIC: {
                if (iter->length !=
                        sizeof(struct acpi_madt_entry_cpu_local_x2apic))
                {
                    printk(LOGLEVEL_INFO,
                           "acpi: Invalid local x2apic madt entry at "
                           "index: %" PRIu32 "\n",
                           index);
                    goto done;
                }

                const struct acpi_madt_entry_cpu_local_x2apic *const hdr =
                    (const struct acpi_madt_entry_cpu_local_x2apic *)iter;

                printk(LOGLEVEL_INFO,
                       "acpi: Found madt-entry local-x2apic\n");

                printk(LOGLEVEL_INFO,
                       "\tACPI Id: %" PRIu32 "\n",
                       hdr->acpi_id);
                printk(LOGLEVEL_INFO,
                       "\tx2ACPI Id: %" PRIu32 "\n",
                       hdr->x2apic_id);
                printk(LOGLEVEL_INFO,
                       "\tFlags: 0x%" PRIx32 "\n",
                       hdr->flags);

                break;
            }
            default:
                printk(LOGLEVEL_INFO,
                       "acpi: Found invalid madt-entry: %" PRIu32 "\n",
                       iter->kind);
                goto done;
        }
    }

done:
    (void)local_apic_base;
    (void)lapic_index;
}