/*
 * kernel/dev/virtio/drivers/block.c
 * © suhas pai
 */

#include "dev/pci/device.h"
#include "dev/printk.h"

#include "block.h"
#include "mmio.h"

enum feature_flags {
    __VIRTIO_BLOCK_HAS_MAX_SIZE = 1ull << 1,
    __VIRTIO_BLOCK_HAS_SEG_MAX = 1ull << 2,
    __VIRTIO_BLOCK_HAS_GEOMETRY = 1ull << 4,
    __VIRTIO_BLOCK_IS_READONLY = 1ull << 5,
    __VIRTIO_BLOCK_HAS_BLOCK_SIZE = 1ull << 6,
    __VIRTIO_BLOCK_CAN_FLUSH_CMD = 1ull << 9,
    __VIRTIO_BLOCK_TOPOLOGY = 1ull << 10,
    __VIRTIO_BLOCK_TOGGLE_CACHE = 1ull << 11,
    __VIRTIO_BLOCK_SUPPORTS_MULTI_QUEUE = 1ull << 12,
    __VIRTIO_BLOCK_SUPPORTS_DISCARD = 1ull << 13,
    __VIRTIO_BLOCK_SUPPORTS_WRITE_ZERO_CMD = 1ull << 14,
    __VIRTIO_BLOCK_GIVES_LIFETIME_INFO = 1ull << 15,
    __VIRTIO_BLOCK_SUPPORTS_SECURE_ERASE_CMD = 1ull << 16,
};

enum legacy_feature_flags {
    __VIRTIO_BLOCK_LEGACY_SUPPORTS_REQ_BARRIERS = 1ull << 0,
    __VIRTIO_BLOCK_LEGACY_SUPPORTS_SCSI = 1ull << 7,
};

struct virtio_block_config {
    le64_t capacity;
    le32_t size_max;
    le32_t seg_max;

    struct virtio_block_geometry {
        le16_t cylinders;

        uint8_t heads;
        uint8_t sectors;
    } geometry;

    le32_t block_size;
    struct virtio_block_topology {
        // # of logical blocks per physical block (log2)
        uint8_t physical_block_exp;
        // offset of first aligned logical block
        uint8_t alignment_offset;
        // suggested minimum I/O size in blocks
        le16_t min_io_size;
        // optimal (suggested maximum) I/O size in blocks
        le32_t opt_io_size;
    } topology;

    uint8_t writeback;
    uint8_t unused0;
    uint16_t num_queues;

    le32_t max_discard_sectors;
    le32_t max_discard_seg;
    le32_t discard_sector_alignment;
    le32_t max_write_zeroes_sectors;
    le32_t max_write_zeroes_seg;

    uint8_t write_zeroes_may_unmap;
    uint8_t unused1[3];

    le32_t max_secure_erase_sectors;
    le32_t max_secure_erase_seg;
    le32_t secure_erase_sector_alignment;
};

struct virtio_device *
virtio_block_driver_init(struct virtio_device *const device) {
    volatile struct virtio_block_config *const config =
        (volatile struct virtio_block_config *)device->device_cfg;

    if (config == NULL) {
        printk(LOGLEVEL_WARN, "virtio-block: device-cfg not found\n");
        return NULL;
    }

    printk(LOGLEVEL_INFO,
           "virtio-block: device has the following info:\n"
           "\tcapacity: 0x%" PRIx64 "\n",
           le_to_cpu(mmio_read(&config->capacity)));

    pci_device_enable_bus_mastering(device->pci_device);
    return NULL;
}