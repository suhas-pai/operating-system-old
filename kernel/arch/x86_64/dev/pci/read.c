/*
 * kernel/arch/x86_64/dev/pci/read.c
 * © suhas pai
 */

#include "dev/pci/pci.h"
#include "dev/port.h"

#include "dev/printk.h"
#include "lib/align.h"

enum pci_config_address_flags {
    __PCI_CONFIG_ADDR_ENABLE = 1ull << 31,
};

static inline void
seek_to_config_space(const struct pci_config_space *const config_space,
                     const uint32_t offset)
{
    const uint32_t address =
        align_down(offset, /*boundary=*/4) |
        ((uint32_t)config_space->function << 8) |
        ((uint32_t)config_space->device_slot << 11)  |
        ((uint32_t)config_space->bus << 16) |
        __PCI_CONFIG_ADDR_ENABLE;

    port_out32(PORT_PCI_CONFIG_ADDRESS, address);
}

uint32_t
arch_pci_read(const struct pci_device_info *const device,
              const uint32_t offset,
              const uint8_t access_size)
{
    seek_to_config_space(&device->config_space, offset);
    switch (access_size) {
        case sizeof(uint8_t):
            return port_in8(PORT_PCI_CONFIG_DATA + (offset & 0b11));
        case sizeof(uint16_t):
            return port_in16(PORT_PCI_CONFIG_DATA + (offset & 0b11));
        case sizeof(uint32_t):
            return port_in32(PORT_PCI_CONFIG_DATA + (offset & 0b11));
    }

    verify_not_reached();
}

bool
arch_pci_write(const struct pci_device_info *const device,
               const uint32_t offset,
               const uint32_t value,
               const uint8_t access_size)
{
    seek_to_config_space(&device->config_space, offset);
    switch (access_size) {
        case sizeof(uint8_t):
            port_out8(PORT_PCI_CONFIG_DATA + (offset & 0b11), value);
            return true;
        case sizeof(uint16_t):
            port_out16(PORT_PCI_CONFIG_DATA + (offset & 0b11), value);
            return true;
        case sizeof(uint32_t):
            port_out32(PORT_PCI_CONFIG_DATA + (offset & 0b11), value);
            return true;
    }

    return false;
}

uint32_t
arch_pcie_read(const struct pci_device_info *const device,
               const uint32_t offset,
               const uint8_t access_size)
{
    const struct pci_domain *const domain = device->domain;
    const uint64_t index = pci_device_get_index(device);

    const struct range mmio_range = mmio_region_get_range(domain->mmio);
    const struct range access_range = range_create(index + offset, access_size);

    if (!range_has_index_range(mmio_range, access_range)) {
        printk(LOGLEVEL_WARN,
               "pcie: attempting to read from index range " RANGE_FMT " which "
               "is outside mmio range of pcie domain, " RANGE_FMT "\n",
               RANGE_FMT_ARGS(access_range),
               RANGE_FMT_ARGS(mmio_range));
        return (uint32_t)-1;
    }

    volatile const void *const spec = device->domain->mmio->base;
    switch (access_size) {
        case sizeof(uint8_t):
            return *reg_to_ptr(volatile uint8_t, spec, offset);
        case sizeof(uint16_t):
            return *reg_to_ptr(volatile uint16_t, spec, offset);
        case sizeof(uint32_t):
            return *reg_to_ptr(volatile uint32_t, spec, offset);
    }

    verify_not_reached();
}

uint32_t
arch_pcie_write(const struct pci_device_info *const device,
                const uint32_t offset,
                const uint32_t value,
                const uint8_t access_size)
{
    volatile const void *const spec = device->domain->mmio->base;
    switch (access_size) {
        case sizeof(uint8_t):
            *reg_to_ptr(volatile uint8_t, spec, offset) = (uint8_t)value;
            return true;
        case sizeof(uint16_t):
            *reg_to_ptr(volatile uint16_t, spec, offset) = (uint16_t)value;
            return true;
        case sizeof(uint32_t):
            *reg_to_ptr(volatile uint32_t, spec, offset) = value;
            return true;
    }

    verify_not_reached();
}