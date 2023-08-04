/*
 * kernel/arch/aarch64/dev/pci/read.c
 * © suhas pai
 */

#include "dev/pci/pci.h"
#include "dev/pci/structs.h"

#include "dev/printk.h"

#include "port.h"

uint32_t
arch_pcie_read(const struct pci_device_info *const device,
               const uint32_t offset,
               const uint8_t access_size)
{
    const struct range access_range = range_create(offset, access_size);
    if (!index_range_in_bounds(access_range,
                               sizeof(struct pci_spec_device_info)))
    {
        printk(LOGLEVEL_WARN,
               "pcie: attempting to write %" PRIu8 " bytes to "
               "offset %" PRIu32 ", which is outside pci-device's descriptor\n",
               access_size,
               offset);
        return (uint32_t)-1;
    }

    volatile const void *const pcie = device->pcie_info;
    switch (access_size) {
        case sizeof(uint8_t):
            return port_in8((port_t)reg_to_ptr(volatile void, pcie, offset));
        case sizeof(uint16_t):
            return port_in16((port_t)reg_to_ptr(volatile void, pcie, offset));
        case sizeof(uint32_t):
            return port_in32((port_t)reg_to_ptr(volatile void, pcie, offset));
    }

    verify_not_reached();
}

bool
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
               "offset %" PRIu32 ", which is outside pci-device's descriptor\n",
               access_size,
               offset);
        return (uint32_t)-1;
    }

    volatile const void *const pcie = device->pcie_info;
    switch (access_size) {
        case sizeof(uint8_t):
            assert(value <= UINT8_MAX);
            port_out8((port_t)reg_to_ptr(volatile void, pcie, offset), value);
            return true;
        case sizeof(uint16_t):
            assert(value <= UINT16_MAX);
            port_out16((port_t)reg_to_ptr(volatile void, pcie, offset), value);
            return true;
        case sizeof(uint32_t):
            port_out32((port_t)reg_to_ptr(volatile void, pcie, offset), value);
            return true;
    }

    verify_not_reached();
}

uint32_t
arch_pci_read(const struct pci_device_info *const device,
              const uint32_t offset,
              const uint8_t access_size)
{
    return arch_pcie_read(device, offset, access_size);
}

bool
arch_pci_write(const struct pci_device_info *const device,
               const uint32_t offset,
               const uint32_t value,
               const uint8_t access_size)
{
    return arch_pcie_write(device, offset, value, access_size);
}
