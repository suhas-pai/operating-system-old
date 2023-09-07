/*
 * kernel/arch/aarch64/dev/pci/read.c
 * © suhas pai
 */

#include "dev/pci/pci.h"
#include "dev/pci/structs.h"

#include "dev/printk.h"
#include "mmio.h"

__optimize(3) uint32_t
arch_pcie_read(const struct pci_device_info *const device,
               const uint32_t offset,
               const uint8_t access_size)
{
    const struct range access_range = range_create(offset, access_size);
    if (!index_range_in_bounds(access_range,
                               sizeof(struct pci_spec_device_info)))
    {
        printk(LOGLEVEL_WARN,
               "pcie: attempting to read %" PRIu8 " bytes at "
               "offset %" PRIu32 ", which falls outside pci-device's "
               "descriptor\n",
               access_size,
               offset);
        return (uint32_t)-1;
    }

    switch (access_size) {
        case sizeof(uint8_t):
            return mmio_read_8(device->pcie_info + offset);
        case sizeof(uint16_t):
            return mmio_read_16(device->pcie_info + offset);
        case sizeof(uint32_t):
            return mmio_read_32(device->pcie_info + offset);
    }

    verify_not_reached();
}

__optimize(3) bool
arch_pcie_write(const struct pci_device_info *const device,
                const uint32_t offset,
                const uint32_t value,
                const uint8_t access_size)
{
    const struct range access_range = range_create(offset, access_size);
    if (!index_range_in_bounds(access_range,
                               sizeof(struct pci_spec_device_info)))
    {
        printk(LOGLEVEL_WARN,
               "pcie: attempting to read %" PRIu8 " bytes from "
               "offset %" PRIu32 ", which falls outside pci-device's "
               "descriptor\n",
               access_size,
               offset);
        return (uint32_t)-1;
    }

    switch (access_size) {
        case sizeof(uint8_t):
            assert(value <= UINT8_MAX);
            mmio_write_8(device->pcie_info + offset, value);

            return true;
        case sizeof(uint16_t):
            assert(value <= UINT16_MAX);
            mmio_write_16(device->pcie_info + offset, value);

            return true;
        case sizeof(uint32_t):
            mmio_write_32(device->pcie_info + offset, value);
            return true;
    }

    verify_not_reached();
}

__optimize(3) uint32_t
arch_pci_read(const struct pci_device_info *const device,
              const uint32_t offset,
              const uint8_t access_size)
{
    return arch_pcie_read(device, offset, access_size);
}

__optimize(3) bool
arch_pci_write(const struct pci_device_info *const device,
               const uint32_t offset,
               const uint32_t value,
               const uint8_t access_size)
{
    return arch_pcie_write(device, offset, value, access_size);
}
