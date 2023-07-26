/*
 * kernel/mm/mmio.h
 * © suhas pai
 */

#pragma once

#include "lib/adt/range.h"
#include "lib/list.h"

struct mmio_region {
    struct list list;

    volatile void *base;
    uint32_t size;
};

struct mmio_region *vmap_mmio(struct range phys_range, uint8_t prot);
struct range mmio_region_get_range(const struct mmio_region *region);


