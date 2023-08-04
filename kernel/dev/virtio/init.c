/*
 * kernel/dev/virtio/init.c
 * © suhas pai
 */

#include "dev/dtb/dtb.h"

#include "dev/printk.h"
#include "dev/driver.h"

#include "lib/align.h"
#include "mm/kmalloc.h"

#include "device.h"

static struct list device_list = LIST_INIT(device_list);
static uint64_t device_count = 0;

static bool virtio_init(volatile struct virtio_mmio_device *const device) {
    if (device->magic != VIRTIO_DEVICE_MAGIC) {
        printk(LOGLEVEL_WARN,
               "virtio-mmio: device (at %p) has an invalid magic "
               "value: %" PRIu32 "\n",
               device,
               device->magic);
        return false;
    }

    if (device->version != 0x1 && device->version != 0x2) {
        printk(LOGLEVEL_WARN,
               "virtio-mmio: device (at %p) has an invalid version "
               "value: %" PRIu32 "\n",
               device,
               device->version);
        return false;
    }

    if (device->device_id == VIRTIO_DEVICE_KIND_INVALID) {
        // Device is not present.
        return false;
    }

    printk(LOGLEVEL_INFO, "virtio-mmio: device at %p recognized\n", device);
    return true;
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
            result = "memory-ballooning-traditional";
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

static bool init_virtio_from_dtb(const void *const dtb, const int nodeoff) {
    struct dtb_addr_size_pair base_addr_reg = {0};
    uint32_t pair_count = 1;

    const bool get_base_addr_reg_result =
        dtb_get_reg_pairs(dtb,
                          nodeoff,
                          /*start_index=*/0,
                          &pair_count,
                          &base_addr_reg);

    if (!get_base_addr_reg_result) {
        printk(LOGLEVEL_WARN,
               "virtio-mmio: base-addr reg of 'reg' property of dtb-node is "
               "is malformed\n");
        return false;
    }

    if (!has_align(base_addr_reg.address, PAGE_SIZE)) {
        printk(LOGLEVEL_WARN,
               "virtio-mmio: base-address of dtb node is not aligned to "
               "page-size\n");
        return false;
    }

    uint64_t size = 0;
    if (!align_up(base_addr_reg.size, PAGE_SIZE, &size)) {
        printk(LOGLEVEL_WARN,
               "virtio-mmio: size (%" PRIu64 "could not be aligned up to "
               "page-size\n",
               base_addr_reg.size);
        return false;
    }

    const struct range phys_range = range_create(base_addr_reg.address, size);
    struct mmio_region *const mmio =
        vmap_mmio(phys_range, PROT_READ | PROT_WRITE, /*flags=*/0);

    if (mmio == NULL) {
        printk(LOGLEVEL_WARN, "virtio-mmio: failed to mmio-map region\n");
        return false;
    }

    if (!virtio_init(mmio->base)) {
        vunmap_mmio(mmio);
    }

    return true;
}

static void init_from_pci(struct pci_device_info *const pci_device) {
    struct range device_id_range = range_create_end(0x1040, 0x107f);
    const char *kind = NULL;

    switch ((enum virtio_pci_trans_device_kind)pci_device->id) {
        case VIRTIO_PCI_TRANS_DEVICE_KIND_NETWORK_CARD:
            kind = "network-card";
            break;
        case VIRTIO_PCI_TRANS_DEVICE_KIND_BLOCK_DEVICE:
            kind = "block-device";
            break;
        case VIRTIO_PCI_TRANS_DEVICE_KIND_MEM_BALLOON_TRAD:
            kind = "memory-ballooning-trad";
            break;
        case VIRTIO_PCI_TRANS_DEVICE_KIND_CONSOLE:
            kind = "console";
            break;
        case VIRTIO_PCI_TRANS_DEVICE_KIND_SCSI_HOST:
            kind = "scsi-host";
            break;
        case VIRTIO_PCI_TRANS_DEVICE_KIND_ENTROPY_SRC:
            kind = "entropy-source";
            break;
        case VIRTIO_PCI_TRANS_DEVICE_KIND_9P_TRANSPORT:
            kind = "9p-transport";
            break;
    }

    if (kind == NULL) {
        const enum virtio_device_kind device_id = pci_device->id;
        if (!range_has_loc(device_id_range, 0x1040 + device_id)) {
            printk(LOGLEVEL_WARN,
                   "virtio-pci: device-id (0x%" PRIx16 ") is invalid\n",
                   device_id);
            return;
        }

        kind = device_kind_string(device_id);
    }

    const bool is_trans = pci_device->revision_id == 0;
    printk(LOGLEVEL_INFO,
           "virtio-pci: recognized %s%s\n",
           kind,
           is_trans ? " (transitional)" : "");

    struct virtio_device *const virt_device = kmalloc(sizeof(*virt_device));
    if (pci_device == NULL) {
        printk(LOGLEVEL_WARN, "virtio-pci: failed to allocate device-info\n");
        return;
    }

    const uint8_t cap_count = array_item_count(pci_device->vendor_cap_list);
    if (cap_count == 0) {
        printk(LOGLEVEL_WARN, "virtio-pci: device has no capabilities\n");
        return;
    }

    list_init(&virt_device->list);

    virt_device->cap_list =
        kmalloc(sizeof(struct virtio_pci_cap_info) * cap_count);

    if (virt_device->cap_list == NULL) {
        printk(LOGLEVEL_WARN, "virtio-pci: failed to allocate cap-list\n");
        kfree(virt_device);

        return;
    }

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
        struct virtio_pci_cap_info *const cap =
            virt_device->cap_list + cap_index;

        if (bar_index > 5) {
            printk(LOGLEVEL_WARN,
                   "\tindex of bar of capability %" PRIu8 " > 5\n",
                   cap_index);

            cap_index++;
            continue;
        }

        if (!index_in_bounds(bar_index, pci_device->max_bar_count)) {
            printk(LOGLEVEL_INFO,
                   "\tindex of base-address-reg (%" PRIu8 ") bar is past end "
                   "of pci-device's bar list\n",
                   bar_index);

            cap_index++;
            continue;
        }

        cap->bar = pci_device->bar_list + bar_index;
        if (!cap->bar->is_present) {
            printk(LOGLEVEL_INFO,
                   "\tbase-address-reg (%" PRIu8 ") bar is not present\n",
                   bar_index);

            cap_index++;
            continue;
        }

        cap->cfg_type = pci_read_virtio_cap_field(cap.cfg_type);
        cap->id = pci_read_virtio_cap_field(cap.id);
        cap->offset = pci_read_virtio_cap_field(cap.offset);
        cap->length = pci_read_virtio_cap_field(cap.length);

        if (cap_len >= sizeof(struct virtio_pci_cap64)) {
            cap->offset |= (uint64_t)pci_read_virtio_cap_field(offset_hi) << 32;
            cap->length |= (uint64_t)pci_read_virtio_cap_field(length_hi) << 32;
        }

        const char *cfg_kind = NULL;
        switch (cap->cfg_type) {
            case VIRTIO_PCI_CAP_COMMON_CFG:
                cfg_kind = "common-cfg";
                break;
            case VIRTIO_PCI_CAP_NOTIFY_CFG:
                cfg_kind = "notify-cfg";
                break;
            case VIRTIO_PCI_CAP_ISR_CFG:
                cfg_kind = "isr-cfg";
                break;
            case VIRTIO_PCI_CAP_DEVICE_CFG:
                cfg_kind = "device-cfg";
                break;
            case VIRTIO_PCI_CAP_PCI_CFG:
                cfg_kind = "pci-cfg";
                break;
            case VIRTIO_PCI_CAP_SHARED_MEMORY_CFG:
                cfg_kind = "shared-memory-cfg";
                break;
            case VIRTIO_PCI_CAP_VENDOR_CFG:
                cfg_kind = "vendor-cfg";
                break;
        }

        const struct range index_range = range_create(cap->offset, cap->length);
        const struct range interface_range =
            subrange_to_full(
                cap->bar->is_mmio ?
                    mmio_region_get_range(cap->bar->mmio) :
                    cap->bar->port_range,
                index_range);

        printk(LOGLEVEL_INFO,
               "\tcapability %" PRIu8 ": %s\n"
               "\t\toffset: 0x%" PRIx64 "\n"
               "\t\tlength: %" PRIu64 "\n"
               "\t\t%s: " RANGE_FMT "\n",
               cap_index,
               cfg_kind,
               cap->offset,
               cap->length,
               cap->bar->is_mmio ? "mmio" : "ports",
               RANGE_FMT_ARGS(interface_range));

        cap_index++;
    }
#undef pci_read_virtio_cap_field

    list_add(&device_list, &virt_device->list);
    device_count++;
}

static const char *const compat_list[] = { "virtio,mmio" };
__driver static const struct driver driver = {
    .dtb = &(struct dtb_driver){
        .compat_count = 1,
        .compat_list = compat_list,
        .init = init_virtio_from_dtb
    },
    .pci = &(struct pci_driver){
        .vendor = 0x1af4,
        .match = PCI_DRIVER_MATCH_VENDOR,
        .init = init_from_pci
    }
};