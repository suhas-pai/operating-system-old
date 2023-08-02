/*
 * kernel/mm/mmio.c
 * © suhas pai
 */

#include "lib/overflow.h"
#include "lib/size.h"

#include "kmalloc.h"
#include "mmio.h"
#include "pagemap.h"

static struct list lower_half_mmio_list = LIST_INIT(lower_half_mmio_list);
static struct list higher_half_mmio_list = LIST_INIT(higher_half_mmio_list);

struct mmio_region *
vmap_mmio(const struct range phys_range,
          const uint8_t prot,
          const uint64_t flags)
{
    assert_msg(range_has_align(phys_range, PAGE_SIZE),
               "mmio phys-range (" RANGE_FMT ") isn't aligned to the page size",
               RANGE_FMT_ARGS(phys_range));

    assert_msg(prot != PROT_NONE,
               "mmio: attempting to map mmio range w/o access permissions");

    assert_msg((prot & PROT_EXEC) == 0,
               "mmio: attempting to map mmio range with execute permissions");

    struct mmio_region *const mmio = kmalloc(sizeof(*mmio));
    assert_msg(mmio != NULL, "mmio: failed to allocate mmio_region");

    uint64_t virt_addr = MMIO_BASE;
    if (flags & __VMAP_MMIO_LOW4G) {
        if (!list_empty(&lower_half_mmio_list)) {
            const struct mmio_region *const prev_mmio =
                list_head(&lower_half_mmio_list, struct mmio_region, list);

            virt_addr =
                check_add_assert((uint64_t)prev_mmio->base, prev_mmio->size);
        } else {
            virt_addr -= HHDM_OFFSET;
        }
    } else {
        if (!list_empty(&higher_half_mmio_list)) {
            const struct mmio_region *const prev_mmio =
                list_head(&higher_half_mmio_list, struct mmio_region, list);

            virt_addr =
                check_add_assert((uint64_t)prev_mmio->base, prev_mmio->size);
        }
    }

    uint64_t virt_end = 0;
    assert_msg(!check_add(virt_addr, phys_range.size, &virt_end),
               "mmio: attempting to map mmio-range that goes past end of "
               "64-bit virtual address space");

    if (flags & __VMAP_MMIO_LOW4G) {
        assert_msg(
            virt_end <= gib(4),
            "mmio: attempting to map mmio-range that goes beyond 4G in lower "
            "4G");
    }

    const bool map_success =
        arch_make_mapping(&kernel_pagemap,
                          /*phys_addr=*/phys_range.front,
                          (uint64_t)virt_addr,
                          phys_range.size,
                          prot,
                          VMA_CACHEKIND_MMIO,
                          /*is_overwrite=*/false);

    assert_msg(map_success, "mmio: failed to map mmio range");

    mmio->base = (volatile void *)virt_addr;
    mmio->size = phys_range.size;

    if (flags & __VMAP_MMIO_LOW4G) {
        list_add(&lower_half_mmio_list, &mmio->list);
    } else {
        list_add(&higher_half_mmio_list, &mmio->list);
    }

    return mmio;
}

bool vunmap_mmio(struct mmio_region *const region) {
    list_delete(&region->list);
    arch_unmap_mapping(&kernel_pagemap, (uint64_t)region->base, region->size);

    return true;
}

struct range mmio_region_get_range(const struct mmio_region *const region) {
    return range_create((uint64_t)region->base, region->size);
}