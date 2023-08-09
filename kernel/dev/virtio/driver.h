/*
 * kernel/dev/virtio/driver.h
 * © suhas pai
 */

#pragma once
#include "device.h"

typedef bool (*virtio_driver_init_t)(struct virtio_device *device);
