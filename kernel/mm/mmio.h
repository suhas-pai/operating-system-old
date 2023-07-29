/*
 * kernel/mm/mmio.h
 * © suhas pai
 */

#pragma once

#include "lib/adt/range.h"
#include "lib/list.h"

#include "mm/mm_types.h"

struct mmio_region {
    struct list list;

    volatile void *base;
    uint32_t size;
};

enum vmap_mmio_flags {
    __VMAP_MMIO_LOW4G = 1 << 0,
};

struct mmio_region *
vmap_mmio(struct range phys_range, uint8_t prot, uint64_t flags);

struct range mmio_region_get_range(const struct mmio_region *region);


