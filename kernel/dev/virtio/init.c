/*
 * kernel/dev/virtio/init.c
 * © suhas pai
 */

#include "dev/driver.h"
#include "dev/printk.h"

#include "drivers/block.h"
#include "drivers/scsi.h"

#include "lib/util.h"

#include "device.h"
#include "mmio.h"

static struct list device_list = LIST_INIT(device_list);
static uint64_t device_count = 0;

static virtio_driver_init_t drivers[] = {
    [VIRTIO_DEVICE_KIND_BLOCK_DEVICE] = virtio_block_driver_init,
    [VIRTIO_DEVICE_KIND_SCSI_HOST] = virtio_scsi_driver_init,
};

static struct string_view device_kind_string[] = {
    [VIRTIO_DEVICE_KIND_INVALID] = SV_STATIC("reserved"),
    [VIRTIO_DEVICE_KIND_NETWORK_CARD] = SV_STATIC("network-card"),
    [VIRTIO_DEVICE_KIND_BLOCK_DEVICE] = SV_STATIC("block-device"),
    [VIRTIO_DEVICE_KIND_CONSOLE] = SV_STATIC("entropy-source"),
    [VIRTIO_DEVICE_KIND_MEM_BALLOON_TRAD] = SV_STATIC("memory-balloon-trad"),
    [VIRTIO_DEVICE_KIND_IOMEM] = SV_STATIC("iomem"),
    [VIRTIO_DEVICE_KIND_RPMSG] = SV_STATIC("rpmsg"),
    [VIRTIO_DEVICE_KIND_SCSI_HOST] = SV_STATIC("scsi-host"),
    [VIRTIO_DEVICE_KIND_9P_TRANSPORT] = SV_STATIC("9p-transport"),
    [VIRTIO_DEVICE_KIND_MAC_80211_WLAN] = SV_STATIC("mac80211-wlan"),
    [VIRTIO_DEVICE_KIND_RPROC_SERIAL] = SV_STATIC("rproc-serial"),
    [VIRTIO_DEVICE_KIND_VIRTIO_CAIF] = SV_STATIC("virtio-caif"),
    [VIRTIO_DEVICE_KIND_MEM_BALLOON] = SV_STATIC("mem-baloon"),
    [VIRTIO_DEVICE_KIND_GPU_DEVICE] = SV_STATIC("gpu-device"),
    [VIRTIO_DEVICE_KIND_TIMER_OR_CLOCK] = SV_STATIC("timer-or-clock"),
    [VIRTIO_DEVICE_KIND_INPUT_DEVICE] = SV_STATIC("input-device"),
    [VIRTIO_DEVICE_KIND_SOCKET_DEVICE] = SV_STATIC("socker-device"),
    [VIRTIO_DEVICE_KIND_CRYPTO_DEVICE] = SV_STATIC("crypto-device"),
    [VIRTIO_DEVICE_KIND_SIGNAL_DISTR_NODULE] =
        SV_STATIC("signal-distribution-module"),
    [VIRTIO_DEVICE_KIND_PSTORE_DEVICE] = SV_STATIC("pstore-device"),
    [VIRTIO_DEVICE_KIND_IOMMU_DEVICE] = SV_STATIC("iommu-device"),
    [VIRTIO_DEVICE_KIND_MEMORY_DEVICE] = SV_STATIC("audio-device"),
    [VIRTIO_DEVICE_KIND_FS_DEVICE] = SV_STATIC("fs-device"),
    [VIRTIO_DEVICE_KIND_PMEM_DEVICE] = SV_STATIC("pmem-device"),
    [VIRTIO_DEVICE_KIND_RPMB_DEVICE] = SV_STATIC("rpmb-device"),
    [VIRTIO_DEVICE_KIND_MAC_80211_HWSIM_WIRELESS_SIM_DEVICE] =
        SV_STATIC("mac80211-hwsim-wireless-simulator-device"),
    [VIRTIO_DEVICE_KIND_VIDEO_ENCODER_DEVICE] =
        SV_STATIC("video-encoder-device"),
    [VIRTIO_DEVICE_KIND_VIDEO_DECODER_DEVICE] =
        SV_STATIC("video-decoder-device"),
    [VIRTIO_DEVICE_KIND_SCMI_DEVICE] = SV_STATIC("scmi-device"),
    [VIRTIO_DEVICE_KIND_NITRO_SECURE_MDOEL] = SV_STATIC("nitro-secure-model"),
    [VIRTIO_DEVICE_KIND_I2C_ADAPTER] = SV_STATIC("i2c-adapter"),
    [VIRTIO_DEVICE_KIND_WATCHDOG] = SV_STATIC("watchdog"),
    [VIRTIO_DEVICE_KIND_CAN_DEVICE] = SV_STATIC("can-device"),
    [VIRTIO_DEVICE_KIND_PARAM_SERVER] = SV_STATIC("parameter-server"),
    [VIRTIO_DEVICE_KIND_AUDIO_POLICY_DEVICE] = SV_STATIC("audio-policy-device"),
    [VIRTIO_DEVICE_KIND_BLUETOOTH_DEVICE] = SV_STATIC("bluetooth-device"),
    [VIRTIO_DEVICE_KIND_GPIO_DEVICE] = SV_STATIC("gpio-device"),
    [VIRTIO_DEVICE_KIND_RDMA_DEVICE] = SV_STATIC("rdma-device")
};

static uint64_t
select_64_bits(volatile uint32_t *const select,
               volatile const uint32_t *const ptr)
{
    mmio_write(select, 0);
    const uint32_t lower = mmio_read(ptr);
    mmio_write(select, 1);
    const uint32_t upper = mmio_read(ptr);

    return ((uint64_t)upper << 32 | lower);
}

static
struct virtio_device *virtio_pci_init(struct virtio_device *const device) {
    if (drivers[device->kind] == NULL) {
        printk(LOGLEVEL_WARN, "virtio-pci: ignoring device, no driver found\n");
        return NULL;
    }

    volatile struct virtio_pci_common_cfg *const cfg = device->common_cfg;

    // 1. Reset the device.
    mmio_write(&cfg->device_status, 0);
    // 2. Set the ACKNOWLEDGE status bit: the guest OS has noticed the device.
    mmio_write(&cfg->device_status, VIRTIO_DEVSTATUS_ACKNOWLEDGE);
    // 3. Set the DRIVER status bit: the guest OS knows how to drive the device.
    mmio_write(&cfg->device_status,
               mmio_read(&cfg->device_status) | VIRTIO_DEVSTATUS_DRIVER);

    // 4. Read device feature bits, and write the subset of feature bits
    // understood by the OS and driver to the device. During this step the
    // driver MAY read (but MUST NOT write) the device-specific configuration
    // fields to check that it can support the device before accepting it.

    uint64_t features =
        le_to_cpu(
            select_64_bits(&cfg->device_feature_select, &cfg->device_feature));

    // The transitional driver MUST execute the initialization sequence as
    // described in 3.1 but omitting the steps 5 and 6.

    const bool is_legacy = (features & VIRTIO_DEVFEATURE_VERSION_1) == 0;
    if (!is_legacy) {
        // 5. Set the FEATURES_OK status bit. The driver MUST NOT accept new
        // feature bits after this step.

        mmio_write(&cfg->device_status, VIRTIO_DEVSTATUS_FEATURES_OK);

        // 6. Re-read device status to ensure the FEATURES_OK bit is still set:
        // otherwise, the device does not support our subset of features and the
        // device is unusable.

        features =
            le_to_cpu(
                select_64_bits(&cfg->device_feature_select,
                               &cfg->device_feature));

        if ((features & VIRTIO_DEVSTATUS_FEATURES_OK) == 0) {
            printk(LOGLEVEL_WARN, "virtio-pci: failed to accept features\n");
            return NULL;
        }
    } else {
        printk(LOGLEVEL_INFO, "virtio-pci: device is legacy\n");
    }

    struct virtio_device *const ret_device = drivers[device->kind](device);
    if (ret_device == NULL) {
        mmio_write(&cfg->device_status,
                   mmio_read(&cfg->device_status) | VIRTIO_DEVSTATUS_FAILED);
        return NULL;
    }

    mmio_write(&cfg->device_status,
               mmio_read(&cfg->device_status) | VIRTIO_DEVSTATUS_DRIVER_OK);

    return ret_device;
}

static void init_from_pci(struct pci_device_info *const pci_device) {
    enum virtio_device_kind device_kind = pci_device->id;
    const char *kind = NULL;

    switch ((enum virtio_pci_trans_device_kind)device_kind) {
        case VIRTIO_PCI_TRANS_DEVICE_KIND_NETWORK_CARD:
            kind = "network-card";
            device_kind = VIRTIO_DEVICE_KIND_NETWORK_CARD;

            break;
        case VIRTIO_PCI_TRANS_DEVICE_KIND_BLOCK_DEVICE:
            kind = "block-device";
            device_kind = VIRTIO_DEVICE_KIND_BLOCK_DEVICE;

            break;
        case VIRTIO_PCI_TRANS_DEVICE_KIND_MEM_BALLOON_TRAD:
            kind = "memory-ballooning-trad";
            device_kind = VIRTIO_DEVICE_KIND_MEM_BALLOON_TRAD;

            break;
        case VIRTIO_PCI_TRANS_DEVICE_KIND_CONSOLE:
            kind = "console";
            device_kind = VIRTIO_DEVICE_KIND_CONSOLE;

            break;
        case VIRTIO_PCI_TRANS_DEVICE_KIND_SCSI_HOST:
            kind = "scsi-host";
            device_kind = VIRTIO_DEVICE_KIND_SCSI_HOST;

            break;
        case VIRTIO_PCI_TRANS_DEVICE_KIND_ENTROPY_SRC:
            kind = "entropy-source";
            device_kind = VIRTIO_DEVICE_KIND_ENTROPY_SRC;

            break;
        case VIRTIO_PCI_TRANS_DEVICE_KIND_9P_TRANSPORT:
            kind = "9p-transport";
            device_kind = VIRTIO_DEVICE_KIND_9P_TRANSPORT;

            break;
    }

    if (kind == NULL) {
        const struct range device_id_range = range_create_end(0x1040, 0x107f);
        if (!range_has_index(device_id_range, device_kind)) {
            printk(LOGLEVEL_WARN,
                   "virtio-pci: device-id (0x%" PRIx16 ") is invalid\n",
                   device_kind);
            return;
        }

        kind = device_kind_string[device_kind].begin;
    }

    const bool is_trans = pci_device->revision_id == 0;
    printk(LOGLEVEL_INFO,
           "virtio-pci: recognized %s%s\n",
           kind,
           is_trans ? " (transitional)" : "");

    const uint8_t cap_count = array_item_count(pci_device->vendor_cap_list);
    if (cap_count == 0) {
        printk(LOGLEVEL_WARN, "virtio-pci: device has no capabilities\n");
        return;
    }

    struct virtio_device virt_device = VIRTIO_DEVICE_INIT(virt_device);

    virt_device.kind = device_kind;
    virt_device.is_transitional = is_trans;
    virt_device.pci_device = pci_device;

#define pci_read_virtio_cap_field(field) \
    pci_read_with_offset(pci_device, \
                         *iter, \
                         struct virtio_pci_cap64, \
                         field)

    uint8_t cap_index = 0;
    array_foreach(&pci_device->vendor_cap_list, uint8_t, iter) {
        const uint8_t cap_len = pci_read_virtio_cap_field(cap.cap_len);
        if (cap_len < sizeof(struct virtio_pci_cap)) {
            printk(LOGLEVEL_INFO,
                   "\tvirtio-pci: capability-length (%" PRIu8 ") is too "
                   "short\n",
                   cap_len);

            cap_index++;
            continue;
        }

        const uint8_t bar_index = pci_read_virtio_cap_field(cap.bar);
        if (!index_in_bounds(bar_index, pci_device->max_bar_count)) {
            printk(LOGLEVEL_INFO,
                   "\tvirtio-pci: index of base-address-reg (%" PRIu8 ") bar "
                   "is past end of pci-device's bar list\n",
                   bar_index);

            cap_index++;
            continue;
        }

        struct pci_device_bar_info *const bar =
            &pci_device->bar_list[bar_index];

        if (!bar->is_present) {
            printk(LOGLEVEL_INFO,
                   "\tvirtio-pci: base-address-reg of bar %" PRIu8 " is not "
                   "present\n",
                   bar_index);

            cap_index++;
            continue;
        }

        if (bar->is_mmio) {
            if (!pci_map_bar(bar)) {
                printk(LOGLEVEL_INFO,
                       "\tvirtio-pci: failed to map bar at index %" PRIu8 "\n",
                       bar_index);

                cap_index++;
                continue;
            }
        }

        const uint8_t cfg_type = pci_read_virtio_cap_field(cap.cfg_type);

        uint64_t offset = pci_read_virtio_cap_field(cap.offset);
        uint64_t length = pci_read_virtio_cap_field(cap.length);

        const struct range index_range = range_create(offset, length);
        const struct range io_range =
            bar->is_mmio ? mmio_region_get_range(bar->mmio) : bar->port_range;

        if (!range_has_index_range(io_range, index_range)) {
            printk(LOGLEVEL_WARN,
                   "\tvirtio-pci: capability has an offset+length pair that "
                   "falls outside device's io-range (" RANGE_FMT ")\n",
                   RANGE_FMT_ARGS(io_range));

            cap_index++;
            continue;
        }

        const char *cfg_kind = NULL;
        switch (cfg_type) {
            case VIRTIO_PCI_CAP_COMMON_CFG:
                if (length < sizeof(struct virtio_pci_common_cfg)) {
                    printk(LOGLEVEL_WARN,
                           "\tvirtio-pci: common-cfg capability is too "
                           "short\n");

                    cap_index++;
                    continue;
                }

                virt_device.common_cfg = bar->mmio->base + offset;
                cfg_kind = "common-cfg";

                break;
            case VIRTIO_PCI_CAP_NOTIFY_CFG:
                if (cap_len < sizeof(struct virtio_pci_notify_cfg_cap)) {
                    printk(LOGLEVEL_WARN,
                           "\tvirtio-pci: notify-cfg capability is too "
                           "short\n");

                    cap_index++;
                    continue;
                }

                virt_device.notify_cfg_offset = *iter;
                cfg_kind = "notify-cfg";

                break;
            case VIRTIO_PCI_CAP_ISR_CFG:
                if (cap_len < sizeof(struct virtio_pci_isr_cfg_cap)) {
                    printk(LOGLEVEL_WARN,
                           "\tvirtio-pci: isr-cfg capability is too short\n");

                    cap_index++;
                    continue;
                }

                virt_device.isr_cfg_offset = *iter;
                cfg_kind = "isr-cfg";

                break;
            case VIRTIO_PCI_CAP_DEVICE_CFG:
                virt_device.device_cfg = bar->mmio->base + offset;
                cfg_kind = "device-cfg";

                break;
            case VIRTIO_PCI_CAP_PCI_CFG:
                if (cap_len < sizeof(struct virtio_pci_cap)) {
                    printk(LOGLEVEL_WARN,
                           "\tvirtio-pci: pci-cfg capability is too short\n");

                    cap_index++;
                    continue;
                }

                virt_device.pci_cfg_offset = *iter;
                cfg_kind = "pci-cfg";

                break;
            case VIRTIO_PCI_CAP_SHARED_MEMORY_CFG:
                offset |= (uint64_t)pci_read_virtio_cap_field(offset_hi) << 32;
                length |= (uint64_t)pci_read_virtio_cap_field(length_hi) << 32;

                const struct virtio_device_shmem_region region = {
                    .phys_range = range_create(offset, length),
                    .id = pci_read_virtio_cap_field(cap.id),
                    .mapped = false
                };

                if (!array_append(&virt_device.shmem_regions, &region)) {
                    printk(LOGLEVEL_WARN,
                           "virtio-pci: failed to add shmem region to "
                           "array\n");

                    cap_index++;
                    continue;
                }

                cfg_kind = "shared-memory-cfg";
                break;
            case VIRTIO_PCI_CAP_VENDOR_CFG:
                if (!array_append(&virt_device.vendor_cfg_list, iter)) {
                    printk(LOGLEVEL_WARN,
                           "virtio-pci: failed to add vendor-cfg to array\n");

                    cap_index++;
                    continue;
                }

                cfg_kind = "vendor-cfg";
                break;
        }

        const struct range interface_range =
            subrange_to_full(io_range, index_range);

        printk(LOGLEVEL_INFO,
               "\tcapability %" PRIu8 ": %s\n"
               "\t\toffset: 0x%" PRIx64 "\n"
               "\t\tlength: %" PRIu64 "\n"
               "\t\t%s: " RANGE_FMT "\n",
               cap_index,
               cfg_kind,
               offset,
               length,
               bar->is_mmio ? "mmio" : "ports",
               RANGE_FMT_ARGS(interface_range));

        cap_index++;
    }

    if (virt_device.common_cfg == NULL) {
        printk(LOGLEVEL_WARN, "virtio-pci: device is missing a common-cfg\n");
        return;
    }

    virtio_pci_init(&virt_device);
#undef pci_read_virtio_cap_field

    list_add(&device_list, &virt_device.list);
    device_count++;
}

static struct pci_driver pci_driver = {
    .vendor = 0x1af4,
    .match = PCI_DRIVER_MATCH_VENDOR,
    .init = init_from_pci
};

__driver static const struct driver driver = {
    .dtb = NULL,
    .pci = &pci_driver
};