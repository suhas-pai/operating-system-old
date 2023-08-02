/*
 * kernel/dev/virtio/init.c
 * © suhas pai
 */

#include "dev/dtb/dtb.h"

#include "dev/printk.h"
#include "dev/driver.h"
#include "lib/align.h"

#include "structs.h"

static bool virtio_init(volatile struct virtio_device *const device) {
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

    if (device->device_id == 0) {
        printk(LOGLEVEL_WARN,
               "virtio-mmio: device (at %p) has a device-id of 0\n",
               device);
        return false;
    }

    printk(LOGLEVEL_INFO, "virtio-mmio: device at %p recognized\n", device);
    return true;
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

static const char *const compat_list[] = { "virtio,mmio" };
__driver static const struct driver driver = {
    .dtb = &(struct dtb_driver){
        .compat_count = 1,
        .compat_list = compat_list,
        .init = init_virtio_from_dtb
    }
};