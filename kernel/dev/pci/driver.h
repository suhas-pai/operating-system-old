/*
 * kernel/dev/pci/driver.h
 * © suhas pai
 */

#pragma once
#include "dev/pci/device.h"

struct pci_driver {
    void (*init)(struct pci_device_info *device);
};