/*
 * kernel/dev/virtio/device.h
 * © suhas pai
 */

#pragma once

#include "mm/mmio.h"
#include "structs.h"

struct virtio_pci_cap_info {
    enum virtio_pci_cap_cfg cfg_type : 8; /* Identifies the structure. */

    uint8_t id; /* Multiple capabilities of the same type */
    uint64_t offset; /* Offset within bar. */
    uint64_t length; /* Length of the structure, in bytes. */

    struct pci_device_bar_info *bar; /* Where to find it. */
};

struct virtio_device {
    struct list list;
    struct virtio_pci_cap_info *cap_list;
    struct pci_device_bar_info *bar;

    uint8_t cap_count;
};