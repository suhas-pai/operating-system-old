/*
 * kernel/arch/aarch64/dev/pci/read.c
 * © suhas pai
 */

#include "dev/pci/pci.h"
#include "lib/assert.h"

uint32_t
pci_read(const struct pci_device_info *const device,
         const uint32_t offset,
         const uint8_t access_size)
{
    volatile const struct pci_spec_device_info *const spec = device->pcie_info;
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

bool
pci_write(const struct pci_device_info *const device,
          const uint32_t offset,
          const uint32_t value,
          const uint8_t access_size)
{
    volatile const struct pci_spec_device_info *const spec = device->pcie_info;
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