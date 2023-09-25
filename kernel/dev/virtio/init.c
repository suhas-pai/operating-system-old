/*
 * kernel/dev/virtio/init.c
 * © suhas pai
 */

#include "dev/driver.h"
#include "dev/printk.h"

#include "drivers/scsi.h"
#include "lib/util.h"

#include "device.h"
#include "mmio.h"

static struct list device_list = LIST_INIT(device_list);
static uint64_t device_count = 0;

static virtio_driver_init_t drivers[] = {
    [VIRTIO_DEVICE_KIND_SCSI_HOST] = virtio_scsi_driver_init,
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

    const uint64_t features =
        select_64_bits(&cfg->device_feature_select, &cfg->device_feature);

    if ((features & VIRTIO_DEVFEATURE_VERSION_1) == 0) {
        mmio_write(&cfg->device_status, VIRTIO_DEVSTATUS_FEATURES_OK);
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

static const char *device_kind_string(const enum virtio_device_kind kind) {
    const char *result = NULL;
    switch (kind) {
        case VIRTIO_DEVICE_KIND_INVALID:
            result = "reserved";
            break;
        case VIRTIO_DEVICE_KIND_NETWORK_CARD:
            result = "network-card";
            break;
        case VIRTIO_DEVICE_KIND_BLOCK_DEVICE:
            result = "block-device";
            break;
        case VIRTIO_DEVICE_KIND_CONSOLE:
            result = "console";
            break;
        case VIRTIO_DEVICE_KIND_ENTROPY_SRC:
            result = "entropy-source";
            break;
        case VIRTIO_DEVICE_KIND_MEM_BALLOON_TRAD:
            result = "memory-balloon-traditional";
            break;
        case VIRTIO_DEVICE_KIND_IOMEM:
            result = "io-memory";
            break;
        case VIRTIO_DEVICE_KIND_RPMSG:
            result = "rpmsg";
            break;
        case VIRTIO_DEVICE_KIND_SCSI_HOST:
            result = "scsi-host";
            break;
        case VIRTIO_DEVICE_KIND_9P_TRANSPORT:
            result = "9p-transport";
            break;
        case VIRTIO_DEVICE_KIND_MAC_80211_WLAN:
            result = "mac80211-wlan";
            break;
        case VIRTIO_DEVICE_KIND_RPROC_SERIAL:
            result = "rproc-serial";
            break;
        case VIRTIO_DEVICE_KIND_VIRTIO_CAIF:
            result = "virtio-caif";
            break;
        case VIRTIO_DEVICE_KIND_MEM_BALLOON:
            result = "mem-balloon";
            break;
        case VIRTIO_DEVICE_KIND_GPU_DEVICE:
            result = "gpu-device";
            break;
        case VIRTIO_DEVICE_KIND_TIMER_OR_CLOCK:
            result = "timer-or-clock";
            break;
        case VIRTIO_DEVICE_KIND_INPUT_DEVICE:
            result = "input-device";
            break;
        case VIRTIO_DEVICE_KIND_SOCKET_DEVICE:
            result = "socket-device";
            break;
        case VIRTIO_DEVICE_KIND_CRYPTO_DEVICE:
            result = "crypto-device";
            break;
        case VIRTIO_DEVICE_KIND_SIGNAL_DISTR_NODULE:
            result = "signal-distribution-module";
            break;
        case VIRTIO_DEVICE_KIND_PSTORE_DEVICE:
            result = "pstore-device";
            break;
        case VIRTIO_DEVICE_KIND_IOMMU_DEVICE:
            result = "iommu-device";
            break;
        case VIRTIO_DEVICE_KIND_MEMORY_DEVICE:
            result = "memory-device";
            break;
        case VIRTIO_DEVICE_KIND_AUDIO_DEVICE:
            result = "audio-device";
            break;
        case VIRTIO_DEVICE_KIND_FS_DEVICE:
            result = "fs-device";
            break;
        case VIRTIO_DEVICE_KIND_PMEM_DEVICE:
            result = "pmem-device";
            break;
        case VIRTIO_DEVICE_KIND_RPMB_DEVICE:
            result = "rpmb-device";
            break;
        case VIRTIO_DEVICE_KIND_MAC_80211_HWSIM_WIRELESS_SIM_DEVICE:
            result = "mac80211-hwsim-wireless-simulator-device";
            break;
        case VIRTIO_DEVICE_KIND_VIDEO_ENCODER_DEVICE:
            result = "video-encoder-device";
            break;
        case VIRTIO_DEVICE_KIND_VIDEO_DECODER_DEVICE:
            result = "video-decoder-device";
            break;
        case VIRTIO_DEVICE_KIND_SCMI_DEVICE:
            result = "scmi-device";
            break;
        case VIRTIO_DEVICE_KIND_NITRO_SECURE_MDOEL:
            result = "nitro-secure-module";
            break;
        case VIRTIO_DEVICE_KIND_I2C_ADAPTER:
            result = "i2c-adapter";
            break;
        case VIRTIO_DEVICE_KIND_WATCHDOG:
            result = "watchdog";
            break;
        case VIRTIO_DEVICE_KIND_CAN_DEVICE:
            result = "can-device";
            break;
        case VIRTIO_DEVICE_KIND_PARAM_SERVER:
            result = "parameter-server";
            break;
        case VIRTIO_DEVICE_KIND_AUDIO_POLICY_DEVICE:
            result = "audio-policy-device";
            break;
        case VIRTIO_DEVICE_KIND_BLUETOOTH_DEVICE:
            result = "bluetooth-device";
            break;
        case VIRTIO_DEVICE_KIND_GPIO_DEVICE:
            result = "gpio-device";
            break;
        case VIRTIO_DEVICE_KIND_RDMA_DEVICE:
            result = "rdma-device";
            break;
    }

    return result;
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

        kind = device_kind_string(device_kind);
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
                   "\tcapability-length (%" PRIu8 ") is too short\n",
                   cap_len);

            cap_index++;
            continue;
        }

        const uint8_t bar_index = pci_read_virtio_cap_field(cap.bar);
        if (!index_in_bounds(bar_index, pci_device->max_bar_count)) {
            printk(LOGLEVEL_INFO,
                   "\tindex of base-address-reg (%" PRIu8 ") bar is past end "
                   "of pci-device's bar list\n",
                   bar_index);

            cap_index++;
            continue;
        }

        struct pci_device_bar_info *const bar =
            pci_device->bar_list + bar_index;

        if (!bar->is_present) {
            printk(LOGLEVEL_INFO,
                   "\tbase-address-reg of bar %" PRIu8 " is not present\n",
                   bar_index);

            cap_index++;
            continue;
        }

        if (bar->is_mmio) {
            if (!pci_map_bar(bar)) {
                printk(LOGLEVEL_INFO,
                       "\tfailed to map bar at index %" PRIu8 "\n",
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
                   "\tcapability has an offset+length pair that falls outside "
                   "device's io-range (" RANGE_FMT ")\n",
                   RANGE_FMT_ARGS(io_range));

            cap_index++;
            continue;
        }

        const char *cfg_kind = NULL;
        switch (cfg_type) {
            case VIRTIO_PCI_CAP_COMMON_CFG:
                if (length < sizeof(struct virtio_pci_common_cfg)) {
                    printk(LOGLEVEL_WARN,
                           "\tcommon-cfg capability is too short\n");

                    cap_index++;
                    continue;
                }

                virt_device.common_cfg = bar->mmio->base + offset;
                cfg_kind = "common-cfg";

                break;
            case VIRTIO_PCI_CAP_NOTIFY_CFG:
                if (cap_len < sizeof(struct virtio_pci_notify_cfg_cap)) {
                    printk(LOGLEVEL_WARN,
                           "\tnotify-cfg capability is too short\n");

                    cap_index++;
                    continue;
                }

                virt_device.notify_cfg_offset = *iter;
                cfg_kind = "notify-cfg";

                break;
            case VIRTIO_PCI_CAP_ISR_CFG:
                if (cap_len < sizeof(struct virtio_pci_isr_cfg_cap)) {
                    printk(LOGLEVEL_WARN,
                           "\tisr-cfg capability is too short\n");

                    cap_index++;
                    continue;
                }

                virt_device.isr_cfg_offset = *iter;
                cfg_kind = "isr-cfg";

                break;
            case VIRTIO_PCI_CAP_DEVICE_CFG:
                cfg_kind = "device-cfg";
                break;
            case VIRTIO_PCI_CAP_PCI_CFG:
                if (cap_len < sizeof(struct virtio_pci_cap)) {
                    printk(LOGLEVEL_WARN,
                           "\tpci-cfg capability is too short\n");

                    cap_index++;
                    continue;
                }

                virt_device.pci_cfg_offset = *iter;
                cfg_kind = "pci-cfg";

                break;
            case VIRTIO_PCI_CAP_SHARED_MEMORY_CFG:
                offset |= (uint64_t)pci_read_virtio_cap_field(offset_hi) << 32;
                length |= (uint64_t)pci_read_virtio_cap_field(length_hi) << 32;

                struct mmio_region *const mmio =
                    vmap_mmio(range_create(offset, length),
                              PROT_READ | PROT_WRITE,
                              /*flags=*/0);

                if (mmio == NULL) {
                    printk(LOGLEVEL_WARN,
                           "virtio-pci: failed to map shared-memory region\n");

                    cap_index++;
                    continue;
                }

                const struct virtio_device_shmem_region region = {
                    .mmio = mmio,
                    .id = pci_read_virtio_cap_field(cap.id)
                };

                if (!array_append(&virt_device.shmem_regions, &region)) {
                    printk(LOGLEVEL_WARN,
                           "virtio-pci: failed to add shmem region to array\n");

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

__driver static const struct driver driver = {
    .pci = &(struct pci_driver){
        .vendor = 0x1af4,
        .match = PCI_DRIVER_MATCH_VENDOR,
        .init = init_from_pci
    }
};