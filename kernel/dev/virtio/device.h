/*
 * kernel/dev/virtio/device.h
 * © suhas pai
 */

#pragma once

#include "mm/mmio.h"
#include "structs.h"

struct virtio_device {
    struct list list;

    struct pci_device_info *pci_device;
    volatile struct virtio_pci_common_cfg *common_cfg;

    uint8_t pci_cfg_offset;
    uint8_t notify_cfg_offset;
    uint8_t isr_cfg_offset;
};