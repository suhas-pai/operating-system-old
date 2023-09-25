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

struct virtio_pci_cap {
    struct pci_spec_capability cap;
    uint8_t cap_len;
    uint8_t cfg_type; /* Identifies the structure. */
    uint8_t bar; /* Where to find it. */
    uint8_t id; /* Multiple capabilities of the same type */
    uint8_t padding[2]; /* Pad to full dword. */
    uint32_t offset; /* Offset within bar. */
    uint32_t length; /* Length of the structure, in bytes. */
} __packed;

struct virtio_pci_cap64 {
    struct virtio_pci_cap cap;
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

struct virtio_pci_notify_cfg_cap {
    struct virtio_pci_cap cap;
    uint32_t notify_off_multiplier; /* Multiplier for queue_notify_off. */
} __packed;

struct virtio_pci_cfg_cap {
    /*
     * The fields cap.bar, cap.length, cap.offset and pci_cfg_data are
     * read-write (RW) for the driver.
     *
     * To access a device region, the driver writes into the capability
     * structure (ie. within the PCI configuration space) as follows:
     *  • The driver sets the BAR to access by writing to cap.bar.
     *  • The driver sets the size of the access by writing 1, 2 or 4 to
     *    cap.length.
     *  • The driver sets the offset within the BAR by writing to cap.offset.
     *
     * At that point, pci_cfg_data will provide a window of size cap.length into
     * the given cap.bar at offset cap.offset.
     */

    struct virtio_pci_cap cap;
    uint8_t pci_cfg_data[4]; /* Data for BAR access. */
} __packed;

struct virtio_pci_isr_cfg_cap {
    struct virtio_pci_cap cap;

    /*
     * To avoid an extra access, simply reading this register resets it to 0 and
     * causes the device to de-assert the interrupt.
     *
     * In this way, driver read of ISR status causes the device to de-assert an
     * interrupt.
     */

    uint8_t data[];
} __packed;

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

enum virtio_device_feature_bits {
    /*
     * Negotiating this feature indicates that the driver can use descriptors
     * with the VIRTQ_DESC_F_INDIRECT flag set, as described in 2.7.5.3 Indirect
     * Descriptors and 2.8.7 Indirect Flag: Scatter-Gather Support.
     */

    VIRTIO_DEVFEATURE_INDR_DESC = 1ull << 28,

    /*
     * This feature enables the used_event and the avail_event fields as
     * described in 2.7.7, 2.7.8 and 2.8.10
     */

    VIRTIO_DEVFEATURE_EVENT_IDX = 1ull << 29,

    /*
     * This indicates compliance with this specification, giving a simple way to
     * detect legacy devices or drivers.
     */

    VIRTIO_DEVFEATURE_VERSION_1 = 1ull << 32,

    /*
     * This feature indicates that the device can be used on a platform where
     * device access to data in memory is limited and/or translated.
     *
     * E.g. this is the case if the device can be located behind an IOMMU that
     * translates bus addresses from the device into physical addresses in
     * memory, if the device can be limited to only access certain memory
     * addresses or if special commands such as a cache flush can be needed to
     * synchronise data in memory with the device.
     *
     * Whether accesses are actually limited or translated is described by
     * platform-specific means.
     * If this feature bit is set to 0, then the device has same access to
     * memory addresses supplied to it as the driver has.
     * In particular, the device will always use physical addresses matching
     * addresses used by the driver (typically meaning physical addresses used
     * by the CPU) and not translated further, and can access any address
     * supplied to it by the driver.
     *
     * When clear, this overrides any platform-specific description of whether
     * device access is limited or translated in any way, e.g. whether an IOMMU
     * may be present.
     */

    VIRTIO_DEVFEATURE_ACCESS_PLATFORM = 1ull << 33,

    /*
     * This feature indicates support for the packed virtqueue layout as
     * described in 2.8 Packed Virtqueues.
     */

    VIRTIO_DEVFEATURES_PACKED_RING = 1ull << 34,

    /*
     * This feature indicates that all buffers are used by the device in the
     * same order in which they have been made available.
     */

    VIRTIO_DEVFEATURES_IN_ORDER = 1ull << 35,

    /*
     * This feature indicates that memory accesses by the driver and the device
     * are ordered in a way described by the platform.
     * If this feature bit is negotiated, the ordering in effect for any memory
     * accesses by the driver that need to be ordered in a specific way with
     * respect to accesses by the device is the one suitable for devices
     * described by the platform.
     * This implies that the driver needs to use memory barriers suitable for
     * devices described by the platform; e.g. for the PCI transport in the case
     * of hardware PCI devices.
     * If this feature bit is not negotiated, then the device and driver are
     * assumed to be implemented in software, that is they can be assumed to run
     * on identical CPUs in an SMP configuration.
     * Thus a weaker form of memory barriers is sufficient to yield better
     * performance.
     */

    VIRTIO_DEVFEATURES_ORDER_PLATFORM = 1ull << 37,

    /*
     * This feature indicates that the device supports Single Root I/O
     * Virtualization. Currently only PCI devices support this feature.
     */

    VIRTIO_DEVFEATURES_SR_IOV = 1ull << 38,

    /*
     * This feature indicates that the driver passes extra data (besides
     * identifying the virtqueue) in its device notifications.
     * See 2.9 Driver Notifications.
     */

    VIRTIO_DEVFEATURES_NOTIFICATION_DATA = 1ull << 39,

    /*
     * This feature indicates that the driver uses the data provided by the
     * device as a virtqueue identifier in available buffer notifications.
     * As mentioned in section 2.9, when the driver is required to send an
     * available buffer notification to the device, it sends the virtqueue
     * number to be notified.
     * The method of delivering notifications is transport specific.
     * With the PCI transport, the device can optionally provide a per-virtqueue
     * value for the driver to use in driver notifications, instead of the
     * number.
     * Some devices may benefit from this flexibility by providing, for example,
     * an internal virtqueue identifier, or an internal offset related to the
     * virtqueue number.
     * This feature indicates the availability of such value.
     * The definition of the data to be provided in driver notification and the
     * delivery method is transport specific.
     * For more details about driver notifications over PCI see 4.1.5.2.
     */

    VIRTIO_DEVFEATURES_NOTIF_CONFIG_DATA = 1ull << 39,

    /*
     * This feature indicates that the driver can reset a queue individually.
     * See 2.6.1
     */

    VIRTIO_DEVFEATURES_RESET = 1ull << 39,
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

    volatile uint32_t queue_num; // Write-only

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

    _Alignas(16) volatile uint32_t queue_notify; // Write-only
    _Alignas(16) volatile const uint32_t interrupt_status;

    volatile uint32_t interrupt_ack; // Write-only

    /*
     * Device status
     *
     * Reading from this register returns the current device status flags.
     * Writing non-zero values to this register sets the status flags,
     * indicating the OS/driver progress.
     *
     * Writing zero (0x0) to this register triggers a device reset. The device
     * sets QueuePFN to zero (0x0) for all queues in the device.
     */

    _Alignas(16) volatile uint32_t status;

    volatile const uint32_t padding_2[35];
    volatile char config_space[];
} __packed;