/*
 * kernel/dev/virtio/structs.h
 * © suhas pai
 */

#pragma once
#include <stdint.h>

#include "dev/pci/structs.h"
#include "lib/macros.h"

enum virtio_pci_trans_device_kind {
    VIRTIO_PCI_TRANS_DEVICE_KIND_NETWORK_CARD = 0x1000,
    VIRTIO_PCI_TRANS_DEVICE_KIND_BLOCK_DEVICE,
    VIRTIO_PCI_TRANS_DEVICE_KIND_MEM_BALLOON_TRAD,
    VIRTIO_PCI_TRANS_DEVICE_KIND_CONSOLE,
    VIRTIO_PCI_TRANS_DEVICE_KIND_SCSI_HOST,
    VIRTIO_PCI_TRANS_DEVICE_KIND_ENTROPY_SRC,
    VIRTIO_PCI_TRANS_DEVICE_KIND_9P_TRANSPORT,
};

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

enum virtio_pci_cap_cfg {
    /* Common configuration */
    VIRTIO_PCI_CAP_COMMON_CFG = 1,
    VIRTIO_PCI_CAP_CFG_MIN = VIRTIO_PCI_CAP_COMMON_CFG,
    /* Notifications */
    VIRTIO_PCI_CAP_NOTIFY_CFG,
    /* ISR Status */
    VIRTIO_PCI_CAP_ISR_CFG,
    /* Device specific configuration */
    VIRTIO_PCI_CAP_DEVICE_CFG,
    /* PCI configuration access */
    VIRTIO_PCI_CAP_PCI_CFG,
    /* Shared memory region */
    VIRTIO_PCI_CAP_SHARED_MEMORY_CFG = 8,
    /* Vendor-specific data */
    VIRTIO_PCI_CAP_VENDOR_CFG,
    VIRTIO_PCI_CAP_CFG_MAX = VIRTIO_PCI_CAP_VENDOR_CFG
};

struct virtio_pci_spec_cap {
    struct pci_spec_capability cap;
    uint8_t cap_len;
    uint8_t cfg_type; /* Identifies the structure. */
    uint8_t bar; /* Where to find it. */
    uint8_t id; /* Multiple capabilities of the same type */
    uint8_t padding[2]; /* Pad to full dword. */
    uint32_t offset; /* Offset within bar. */
    uint32_t length; /* Length of the structure, in bytes. */
} __packed;

struct virtio_pci_spec_cap64 {
    struct virtio_pci_spec_cap cap;
    uint32_t offset_hi;
    uint32_t length_hi;
} __packed;

struct virtio_pci_common_cfg {
    /* About the whole device. */
    uint32_t device_feature_select;
    const uint32_t device_feature;
    uint32_t driver_feature_select;
    uint32_t driver_feature;
    uint16_t config_msix_vector;
    const uint16_t num_queues;
    uint8_t device_status;
    const uint8_t config_generation;

    /* About a specific virtqueue. */
    uint16_t queue_select;
    uint16_t queue_size;
    uint16_t queue_msix_vector;
    uint16_t queue_enable;
    const uint16_t queue_notify_off;
    uint64_t queue_desc;
    uint64_t queue_driver;
    uint64_t queue_device;
    const uint16_t queue_notify_data;
    uint16_t queue_reset;
} __packed;

enum virtio_mmio_device_status {
    /*
     * Indicates that the guest OS has found the device and recognized it as a
     * valid virtio device.
     */

    VIRTIO_MMIO_DEVSTATUS_ACKNOWLEDGE = 1 << 0,

    /* Indicates that the guest OS knows how to drive the device */
    VIRTIO_MMIO_DEVSTATUS_DRIVER = 1 << 1,

    /* Indicates that the driver is set up and ready to drive the device. */
    VIRTIO_MMIO_DEVSTATUS_DRIVER_OK = 1 << 2,

    /*
     * Indicates that the driver has acknowledged all the features it
     * understands, and feature negotiation is complete.
     */

    VIRTIO_MMIO_DEVSTATUS_FEATURES_OK = 1 << 3,

    /*
     * Indicates that the device has experienced an error from which it can’t
     * re-cover.
     */

    VIRTIO_MMIO_DEVSTATUS_DEVICE_NEEDS_RESET = 1 << 6,

    /*
     * Indicates that something went wrong in the guest, and it has given up on
     * the device. This could be an internal error, or the driver didn’t like
     * the device for some reason, or even a fatal error during device
     * operation.
     */

    VIRTIO_MMIO_DEVSTATUS_FAILED = 1 << 7
};

#define VIRTIO_DEVICE_MAGIC 0x74726976

struct virtio_mmio_device {
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

struct virtio_mmio_device_legacy {
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