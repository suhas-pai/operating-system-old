/*
 * kernel/dev/virtio/device.h
 * © suhas pai
 */

#pragma once

#include "lib/adt/array.h"

#include "mm/mmio.h"
#include "structs.h"

struct virtio_device_shmem_region {
    struct mmio_region *mmio;
    uint8_t id;
};

struct virtio_device;
struct virtio_config_space_operations {
    void
    (*read)(struct virtio_device *device,
            uint64_t offset,
            uint64_t size,
            void *buf);

    void
    (*write)(struct virtio_device *device,
             const void *buf,
             uint64_t offset,
             uint64_t size);
};

struct virtio_device {
    struct list list;

    struct pci_device_info *pci_device;
    volatile struct virtio_pci_common_cfg *common_cfg;

    // Array of struct virtio_device_shmem_region
    struct array shmem_regions;

    // Array of uint8_t
    struct array vendor_cfg_list;

    uint8_t pci_cfg_offset;
    uint8_t notify_cfg_offset;
    uint8_t isr_cfg_offset;

    bool is_transitional : 1;
    enum virtio_device_kind kind;

    struct virtio_config_space_operations ops;
};

#define VIRTIO_DEVICE_INIT(name) \
    ((struct virtio_device){ \
        .list = LIST_INIT(name.list), \
        .pci_device = NULL, \
        .common_cfg = NULL, \
        .shmem_regions = ARRAY_INIT(sizeof(struct virtio_device_shmem_region)),\
        .vendor_cfg_list = ARRAY_INIT(sizeof(uint8_t)), \
        .pci_cfg_offset = 0, \
        .notify_cfg_offset = 0, \
        .isr_cfg_offset = 0, \
        .is_transitional = false, \
        .kind = VIRTIO_DEVICE_KIND_INVALID \
    })
