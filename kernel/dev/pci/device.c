/*
 * kernel/dev/pci/device.h
 * © suhas pai
 */

#include "dev/printk.h"
#include "lib/bits.h"

#include "device.h"
#include "structs.h"

#if defined(__x86_64__)
    static void
    bind_msi_to_vector(const struct pci_device_info *const device,
                       const uint64_t address,
                       const isr_vector_t vector,
                       const bool masked)
    {
        volatile struct pci_spec_cap_msi *const msi = device->pcie_msi;
        const uint16_t msg_control = msi->msg_control;

        const bool is_64_bit =
            (msg_control & __PCI_CAPMSI_CTRL_64BIT_CAPABLE) != 0;

        /*
        * If we're supposed to mask the vector, but masking isn't supported,
        * then simply bail.
        */

        const bool supports_masking =
            (msg_control & __PCI_CAPMSI_CTRL_PER_VECTOR_MASK) != 0;

        if (masked && !supports_masking) {
            return;
        }

        msi->msg_address = (uint32_t)address;
        if (is_64_bit) {
            msi->bits64.msg_data = vector;
        } else {
            msi->bits32.msg_data = vector;
        }

        msi->msg_control |= __PCI_CAPMSI_CTRL_ENABLE;
        msi->msg_control &=
            (uint16_t)~__PCI_CAPMSI_CTRL_MULTIMSG_ENABLE;

        if (masked) {
            if (is_64_bit) {
                msi->bits64.mask_bits |= 1ull << vector;
            } else {
                msi->bits32.mask_bits |= 1ul << vector;
            }
        }
    }

    static void
    bind_msix_to_vector(struct pci_device_info *const device,
                        const uint64_t address,
                        const isr_vector_t vector,
                        const bool masked)
    {
        volatile struct pci_spec_cap_msix *const msix = device->pcie_msix;
        const uint64_t msix_vector =
            bitmap_find(&device->msix_table,
                        /*count=*/1,
                        /*start_index=*/0,
                        /*expected_value=*/false,
                        /*invert=*/1);

        if (msix_vector == FIND_BIT_INVALID) {
            printk(LOGLEVEL_WARN,
                   "pcie: no free msix table entry found for binding msix "
                   "vector\n");
            return;
        }

        /*
        * The lower 3 bits of the Table Offset is the BIR.
        *
        * The BIR (Base Index Register) is the index of the BAR that contains
        * the MSI-X Table.
        *
        * The remaining 29 (32-3) bits of the Table Offset is the offset to the
        * MSI-X Table in the BAR.
        */

        const uint32_t table_offset = msix->table_offset;
        const uint8_t bar_index =
            table_offset & F_PCI_SPEC_BAR_TABLE_OFFSET_BIR;

        if (!index_in_bounds(bar_index, device->max_bar_count)) {
            printk(LOGLEVEL_WARN,
                   "pcie: got invalid bar index while trying to bind address "
                   "%p to msix vector " ISR_VECTOR_FMT "\n",
                   (void *)address,
                   vector);
            return;
        }

        if (!device->bar_list[bar_index].is_present) {
            printk(LOGLEVEL_WARN,
                   "pcie: encountered invalid bar index for msix table while "
                   "trying to bind address %p to msix "
                   "vector " ISR_VECTOR_FMT "\n",
                   (void *)address,
                   vector);
            return;
        }

        struct pci_device_bar_info *const bar = &device->bar_list[bar_index];

        const uint64_t bar_address = (uint64_t)bar->mmio->base;
        const uint64_t table_addr = bar_address + table_offset;

        volatile struct pci_spec_cap_msix_table_entry *const table =
            (volatile struct pci_spec_cap_msix_table_entry *)table_addr;

        table[msix_vector].msg_address_lower32 = (uint32_t)address;
        table[msix_vector].msg_address_upper32 = (uint32_t)(address >> 32);
        table[msix_vector].data = vector;
        table[msix_vector].control = masked;

        /* Enable MSI-X at the very end, after we set the table-entry */
        msix->msg_control |= __PCI_CAPMSI_CTRL_ENABLE;
    }

    void
    pci_device_bind_msi_to_vector(struct pci_device_info *const device,
                                  const struct cpu_info *const cpu,
                                  const isr_vector_t vector,
                                  const bool masked)
    {
        const enum pci_device_msi_support msi_support = device->msi_support;
        const uint64_t msi_address = (0xFEE00000 | cpu->lapic_id << 12);

        switch (msi_support) {
            case PCI_DEVICE_MSI_SUPPORT_NONE:
                printk(LOGLEVEL_WARN,
                       "pcie: device " PCI_DEVICE_INFO_FMT " does not support "
                       "msi. failing to bind msi to "
                       "vector " ISR_VECTOR_FMT "\n",
                       PCI_DEVICE_INFO_FMT_ARGS(device),
                       vector);
                return;
            case PCI_DEVICE_MSI_SUPPORT_MSI:
                bind_msi_to_vector(device, msi_address, vector, masked);
                return;
            case PCI_DEVICE_MSI_SUPPORT_MSIX:
                bind_msix_to_vector(device, msi_address, vector, masked);
                return;
        }
    }
#endif /* defined(__x86_64__) */

void pci_device_enable_bus_mastering(struct pci_device_info *const device) {
    if (device->pcie_info != NULL) {
        device->pcie_info->command |= __PCI_DEVCMDREG_BUS_MASTER;
    } else {
        const uint16_t old_command =
            device->group->read(
                device,
                offsetof(struct pci_spec_device_info, command),
                sizeof_field(struct pci_spec_device_info, command));

        const uint16_t new_command =
            old_command | __PCI_DEVCMDREG_BUS_MASTER;

        device->group->write(device,
                             offsetof(struct pci_device_info, command),
                             new_command,
                             sizeof(uint32_t));
    }
}