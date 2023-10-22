/*
 * kernel/dev/virtio/transport.h
 * © suhas pai
 */

#pragma once
#include <stdint.h>

struct virtio_device;
struct virtio_transport_ops {
    uint8_t (*read_device_status)(struct virtio_device *device);
    uint64_t (*read_device_features)(struct virtio_device *device);

    void (*write_device_status)(struct virtio_device *device, uint8_t status);
    void (*write_driver_features)(struct virtio_device *device, uint64_t feat);

    void
    (*read_device_info)(struct virtio_device *device,
                        uint16_t offset,
                        uint8_t size,
                        void *buf);

    void
    (*write_device_info)(struct virtio_device *device,
                         uint16_t offset,
                         uint8_t size,
                         const void *buf);
};

struct virtio_transport_ops virtio_transport_ops_for_mmio();
struct virtio_transport_ops virtio_transport_ops_for_pci();

#define virtio_device_read_status(device) \
    (device)->ops.read_device_status(device)
#define virtio_device_read_features(device) \
    (device)->ops.read_device_features(device)
#define virtio_device_write_status(device, status) \
    (device)->ops.write_device_status(device, status)
#define virtio_device_write_features(device, features) \
    (device)->ops.write_driver_features(device, features)
#define virtio_device_read_info(device, offset, size, buf) \
    (device)->ops.read_device_info(device, offset, size, buf)
#define virtio_device_write_info(device, offset, size, buf) \
    (device)->ops.write_device_info(device, offset, size, buf)

#define virtio_device_read_info_field(device, type, field) \
    ({ \
        typeof_field(type, field) __result__; \
        virtio_device_read_info(device, \
                                offsetof(type, field), \
                                sizeof_field(type, field), \
                                &__result__); \
        __result__; \
    })

#define virtio_device_write_info_field(device, type, field, value) \
    virtio_device_write_info(device, \
                             offsetof(type, field), \
                             sizeof(type, field), \
                             value)
