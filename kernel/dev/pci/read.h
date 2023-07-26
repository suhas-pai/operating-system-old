/*
 * kernel/dev/pci/read.c
 * © suhas pai
 */

#include "pci.h"

uint8_t
pci_config_space_read_u8(const struct pci_config_space *config_spcae,
                         uint8_t offset);

uint16_t
pci_config_space_read_u16(const struct pci_config_space *config_spcae,
                         uint16_t offset);

uint32_t
pci_config_space_read_u32(const struct pci_config_space *config_spcae,
                         uint32_t offset);

uint64_t
pci_config_space_read_u64(const struct pci_config_space *config_spcae,
                         uint64_t offset);

void
pci_config_space_write_u8(const struct pci_config_space *config_spcae,
                          uint8_t offset,
                          uint8_t value);

void
pci_config_space_write_u16(const struct pci_config_space *config_spcae,
                           uint8_t offset,
                           uint16_t value);

void
pci_config_space_write_u32(const struct pci_config_space *config_spcae,
                           uint8_t offset,
                           uint32_t value);

void
pci_config_space_write_u64(const struct pci_config_space *config_spcae,
                           uint8_t offset,
                           uint64_t value);
