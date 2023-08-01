/*
 * kernel/arch/aarch64/dev/pci/read.c
 * © suhas pai
 */

#include "dev/pci/pci.h"
#include "dev/printk.h"

#include "port.h"

uint32_t
arch_pcie_read(const struct pci_device_info *const device,
               const uint32_t offset,
               const uint8_t access_size)
{
    struct pci_domain *const domain = device->domain;
    const uint64_t index =
        (uint64_t)((device->config_space.bus - domain->bus_range.front) << 20) |
        ((uint64_t)device->config_space.device_slot << 15) |
        ((uint64_t)device->config_space.function << 12);

    const struct range mmio_range = mmio_region_get_range(domain->mmio);
    const struct range access_range = range_create(index + offset, access_size);

    if (!range_has_index_range(mmio_range, access_range)) {
        printk(LOGLEVEL_WARN,
               "pcie: attempting to write to index-range " RANGE_FMT " which "
               "is outside mmio range of pcie domain, " RANGE_FMT "\n",
               RANGE_FMT_ARGS(access_range),
               RANGE_FMT_ARGS(mmio_range));
        return (uint32_t)-1;
    }

    volatile const void *const spec = device->domain->mmio->base + index;
    switch (access_size) {
        case sizeof(uint8_t):
            return port_in8((port_t)reg_to_ptr(volatile void, spec, offset));
        case sizeof(uint16_t):
            return port_in16((port_t)reg_to_ptr(volatile void, spec, offset));
        case sizeof(uint32_t):
            return port_in32((port_t)reg_to_ptr(volatile void, spec, offset));
    }

    verify_not_reached();
}

bool
arch_pcie_write(const struct pci_device_info *const device,
                const uint32_t offset,
                const uint32_t value,
                const uint8_t access_size)
{
    struct pci_domain *const domain = device->domain;
    const uint64_t index =
        (uint64_t)((device->config_space.bus - domain->bus_range.front) << 20) |
        ((uint64_t)device->config_space.device_slot << 15) |
        ((uint64_t)device->config_space.function << 12);

    const struct range mmio_range = mmio_region_get_range(domain->mmio);
    const struct range access_range = range_create(index + offset, access_size);

    if (!range_has_index_range(mmio_range, access_range)) {
        printk(LOGLEVEL_WARN,
               "pcie: attempting to write to index-range " RANGE_FMT " which "
               "is outside mmio range of pcie domain, " RANGE_FMT "\n",
               RANGE_FMT_ARGS(access_range),
               RANGE_FMT_ARGS(mmio_range));
        return (uint32_t)-1;
    }

    volatile const void *const spec = device->domain->mmio->base + index;
    switch (access_size) {
        case sizeof(uint8_t):
            port_out8((port_t)reg_to_ptr(volatile void, spec, offset), value);
            return true;
        case sizeof(uint16_t):
            port_out16((port_t)reg_to_ptr(volatile void, spec, offset), value);
            return true;
        case sizeof(uint32_t):
            port_out32((port_t)reg_to_ptr(volatile void, spec, offset), value);
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
