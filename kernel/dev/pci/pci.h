/*
 * kernel/dev/pci/pci.h
 * © suhas pai
 */

#pragma once
#include "device.h"

struct pci_group *
pci_group_create_pcie(struct range bus_range,
                      uint64_t base_addr,
                      uint16_t segment);

void pci_init();