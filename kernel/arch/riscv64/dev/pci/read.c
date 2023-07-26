/*
 * kernel/arch/riscv64/dev/pci/read.c
 * © suhas pai
 */

#include "dev/pci/pci.h"
#include "lib/assert.h"

uint32_t
pci_read(const struct pci_config_space *const config_space,
         const uint32_t offset,
         const uint8_t access_size)
{
    verify_not_reached();

    (void)config_space;
    (void)offset;
    (void)access_size;
}

bool
pci_write(const struct pci_config_space *const config_space,
          const uint32_t offset,
          const uint32_t value,
          const uint8_t access_size)
{
    verify_not_reached();

    (void)config_space;
    (void)offset;
    (void)value;
    (void)access_size;
}