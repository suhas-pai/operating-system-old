/*
 * kernel/dev/virtio/drivers/scsi.h
 * © suhas pai
 */

#pragma once
#include "../device.h"

struct virtio_scsi_host_device {

};

struct virtio_device *virtio_scsi_driver_init(struct virtio_device *device);