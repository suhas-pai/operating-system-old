/*
 * kernel/dev/virtio/structs.h
 * © suhas pai
 */

#pragma once

#include <stdint.h>
#include "lib/macros.h"

enum virtio_device_kind {
    VIRTIO_DEVICE_KIND_INVALID,
    VIRTIO_DEVICE_KIND_NETWORK_CARD,
    VIRTIO_DEVICE_KIND_BLOCK_DEVICE,
    VIRTIO_DEVICE_KIND_CONSOLE,
    VIRTIO_DEVICE_KIND_ENTROPY_SRC,
    VIRTIO_DEVICE_KIND_MEM_BALLOON_TRAD,
    VIRTIO_DEVICE_KIND_IOMEM,
    VIRTIO_DEVICE_KIND_RPMSG,
    VIRTIO_DEVICE_KIND_SCSI_HOST,
    VIRTIO_DEVICE_KIND_9P_TRANSPORT,
    VIRTIO_DEVICE_KIND_MAC_80211_WLAN,
    VIRTIO_DEVICE_KIND_RPROC_SERIAL,
    VIRTIO_DEVICE_KIND_VIRTIO_CAIF,
    VIRTIO_DEVICE_KIND_MEM_BALLOON,
    VIRTIO_DEVICE_KIND_GPU_DEVICE = 16,
    VIRTIO_DEVICE_KIND_TIMER_OR_CLOCK,
    VIRTIO_DEVICE_KIND_INPUT_DEVICE,
    VIRTIO_DEVICE_KIND_SOCKET_DEVICE,
    VIRTIO_DEVICE_KIND_CRYPTO_DEVICE,
    VIRTIO_DEVICE_KIND_SIGNAL_DISTR_NODULE,
    VIRTIO_DEVICE_KIND_PSTORE_DEVICE,
    VIRTIO_DEVICE_KIND_IOMMU_DEVICE,
    VIRTIO_DEVICE_KIND_MEMORY_DEVICE,
    VIRTIO_DEVICE_KIND_AUDIO_DEVICE,
    VIRTIO_DEVICE_KIND_FS_DEVICE,
    VIRTIO_DEVICE_KIND_PMEM_DEVICE,
    VIRTIO_DEVICE_KIND_RPMB_DEVICE,
    VIRTIO_DEVICE_KIND_MAC_80211_HWSIM_WIRELESS_SIM_DEVICE,
    VIRTIO_DEVICE_KIND_VIDEO_ENCODER_DEVICE,
    VIRTIO_DEVICE_KIND_VIDEO_DECODER_DEVICE,
    VIRTIO_DEVICE_KIND_SCMI_DEVICE,
    VIRTIO_DEVICE_KIND_NITRO_SECURE_MDOEL,
    VIRTIO_DEVICE_KIND_I2C_ADAPTER,
    VIRTIO_DEVICE_KIND_WATCHDOG,
    VIRTIO_DEVICE_KIND_CAN_DEVICE,
    VIRTIO_DEVICE_KIND_PARAM_SERVER = 38,
    VIRTIO_DEVICE_KIND_AUDIO_POLICY_DEVICE,
    VIRTIO_DEVICE_KIND_BLUETOOTH_DEVICE,
    VIRTIO_DEVICE_KIND_GPIO_DEVICE,
    VIRTIO_DEVICE_KIND_RDMA_DEVICE,
};

enum virtio_device_status {
    /*
     * Indicates that the guest OS has found the device and recognized it as a
     * valid virtio device.
     */

    VIRTIO_DEVSTATUS_ACKNOWLEDGE = 1 << 0,

    /* Indicates that the guest OS knows how to drive the device */
    VIRTIO_DEVSTATUS_DRIVER = 1 << 1,

    /* Indicates that the driver is set up and ready to drive the device. */
    VIRTIO_DEVSTATUS_DRIVER_OK = 1 << 2,

    /*
     * Indicates that the driver has acknowledged all the features it
     * understands, and feature negotiation is complete.
     */

    VIRTIO_DEVSTATUS_FEATURES_OK = 1 << 3,

    /*
     * Indicates that the device has experienced an error from which it can’t
     * re-cover.
     */

    VIRTIO_DEVSTATUS_DEVICE_NEEDS_RESET = 1 << 6,

    /*
     * Indicates that something went wrong in the guest, and it has given up on
     * the device. This could be an internal error, or the driver didn’t like
     * the device for some reason, or even a fatal error during device
     * operation.
     */

    VIRTIO_DEVSTATUS_FAILED = 1 << 7
};

#define VIRTIO_DEVICE_MAGIC 0x74726976

struct virtio_device {
    volatile const uint32_t magic;
    volatile const uint32_t version;
    volatile const uint32_t device_id;
    volatile const uint32_t vendor_id;
    volatile const uint32_t device_features;

    volatile uint32_t device_features_select; // Write-only
    _Alignas(16) volatile uint32_t driver_features; // Write-only

    volatile uint32_t driver_features_select; // Write-only
    _Alignas(16) volatile uint32_t queue_select; // Write-only

    volatile const uint32_t queue_num_max;
    volatile uint32_t queue_num;              // Write-only
    volatile const uint64_t padding_1;
    volatile uint32_t queue_ready;

    _Alignas(16) volatile uint32_t queue_notify;           // Write-only
    _Alignas(16) volatile const uint32_t interrupt_status;

    volatile uint32_t interrupt_ack; // Write-only

    _Alignas(16) volatile uint32_t status;
    _Alignas(16) volatile uint32_t queue_desc_low; // Write-only

    volatile uint32_t queue_desc_high; // Write-only

    _Alignas(16) volatile uint32_t queue_driver_low; // Write-only
    volatile uint32_t queue_driver_high; // Write-only

    _Alignas(16) volatile uint32_t queue_device_low; // Write-only

    volatile uint32_t queue_device_high; // Write-only
    volatile uint32_t shared_memory_id; // Write-only

    _Alignas(16) volatile const uint32_t shared_memory_length_low;

    volatile const uint32_t shared_memory_length_high;
    volatile const uint32_t shared_memory_base_low;
    volatile const uint32_t shared_memory_base_high;
    volatile uint32_t queue_reset;

    volatile const uint32_t padding_2[14];
    volatile uint32_t config_generation;

    volatile char config_space[];
} __packed;

struct virtio_device_legacy {
    volatile const uint32_t magic;
    volatile const uint32_t version;
    volatile const uint32_t device_id;
    volatile const uint32_t vendor_id;
    volatile const uint32_t host_features;

    volatile uint32_t host_features_select; // Write-only
    _Alignas(16) volatile uint32_t guest_features; // Write-only
    volatile uint32_t guest_features_select; // Write-only

    /*
     * Guest page size
     *
     * The driver writes the guest page size in bytes to the register during
     * initialization, before any queues are used. This value should be a power
     * of 2 and is used by the device to calculate the Guest address of the
     * first queue page (see QueuePFN).
     */

    volatile uint32_t guest_page_size; // Write-only

    /*
     * Virtual queue index
     *
     * Writing to this register selects the virtual queue that the following
     * operations on the QueueNumMax, QueueNum, QueueAlign and QueuePFN
     * registers apply to. The index number of the first queue is zero (0x0).
     */

    _Alignas(16) volatile uint32_t queue_select; // Write-only

    /*
     * Maximum virtual queue size
     *
     * Reading from the register returns the maximum size of the queue the
     * device is ready to process or zero (0x0) if the queue is not available.
     * This applies to the queue selected by writing to QueueSel and is allowed
     * only when QueuePFN is set to zero (0x0), so when the queue is not
     * actively used.
     */

    volatile const uint32_t queue_num_max;

    /*
     * Virtual queue size
     *
     * Queue size is the number of elements in the queue.
     * Writing to this register notifies the device what size of the queue the
     * driver will use. This applies to the queue selected by writing to
     * QueueSel.
     */

    volatile uint32_t queue_num;              // Write-only

    /*
     * Used Ring alignment in the virtual queue
     *
     * Writing to this register notifies the device about alignment boundary of
     * the Used Ring in bytes. This value should be a power of 2 and applies to
     * the queue selected by writing to QueueSel.
     */

    volatile uint32_t queue_align; // Write-only

    /*
     * Writing to this register notifies the device about location of the
     * virtual queue in the Guest’s physical address space. This value is the
     * index number of a page starting with the queue Descriptor Table.
     * Value zero (0x0) means physical address zero (0x00000000) and is illegal.
     *
     * When the driver stops using the queue it writes zero (0x0) to this
     * register. Reading from this register returns the currently used page
     * number of the queue, therefore a value other than zero (0x0) means that
     * the queue is in use. Both read and write accesses apply to the queue
     * selected by writing to QueueSel.
     */

    volatile uint32_t queue_pfn;

    _Alignas(16) volatile uint32_t queue_notify;           // Write-only
    _Alignas(16) volatile const uint32_t interrupt_status;

    volatile uint32_t interrupt_ack; // Write-only

    /*
     * Device status
     *
     * Reading from this register returns the current device status flags.
     * Writing non-zero values to this register sets the status flags,
     * indicating the OS/driver progress.
     * Writing zero (0x0) to this register triggers a device reset. The device
     * sets QueuePFN to zero (0x0) for all queues in the device. Also see
     */

    _Alignas(16) volatile uint32_t status;

    volatile const uint32_t padding_2[35];
    volatile char config_space[];
} __packed;