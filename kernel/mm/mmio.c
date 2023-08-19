/*
 * kernel/mm/mmio.c
 * © suhas pai
 */

#include "dev/printk.h"
#include "lib/size.h"

#include "kmalloc.h"
#include "mmio.h"
#include "pagemap.h"

// FIXME: Use an avltree
static struct list lower_half_mmio_list = LIST_INIT(lower_half_mmio_list);
static struct list higher_half_mmio_list = LIST_INIT(higher_half_mmio_list);

static struct range
get_virt_range(const struct range phys_range, const uint64_t flags) {
#if 0
    const uint64_t guard_pages_size = PAGE_SIZE;

#if !(defined(__riscv) && defined(__LP64__))
    uint64_t virt_addr = MMIO_BASE;
    if (flags & __VMAP_MMIO_LOW4G) {
        if (!list_empty(&lower_half_mmio_list)) {
            const struct mmio_region *const prev_mmio =
                list_head(&lower_half_mmio_list, struct mmio_region, list);

            virt_addr =
                check_add_assert((uint64_t)prev_mmio->base, prev_mmio->size) +
                guard_pages_size;
        } else {
            virt_addr -= HHDM_OFFSET;
        }
    } else {
        if (!list_empty(&higher_half_mmio_list)) {
            const struct mmio_region *const prev_mmio =
                list_head(&higher_half_mmio_list, struct mmio_region, list);

            virt_addr =
                check_add_assert((uint64_t)prev_mmio->base, prev_mmio->size) +
                guard_pages_size;
        }
    }

    uint64_t virt_end = 0;
    if (!check_add(virt_addr, phys_range.size, &virt_end) ||
        !check_add(virt_end, guard_pages_size, &virt_end))
    {
        printk(LOGLEVEL_WARN,
               "mmio: attempting to map mmio-range that goes past end of "
               "64-bit virtual address space\n");
        return range_create_empty();
    }
#else
    // Don't use guard-pages here
    (void)guard_pages_size;

    struct list *list = &higher_half_mmio_list;
    uint64_t virt_addr = phys_range.front;

    if (flags & __VMAP_MMIO_LOW4G) {
        list = &lower_half_mmio_list;
    } else {
        virt_addr = (uint64_t)phys_to_virt(virt_addr);
    }

    struct mmio_region *iter = NULL;
    const struct range virt_range = range_create(virt_addr, phys_range.size);

    list_foreach(iter, list, list) {
        if (range_overlaps(mmio_region_get_range(iter), virt_range)) {
            printk(LOGLEVEL_WARN,
                   "mmio: failing to mmio map requested range because it "
                   "overlaps with an already existing map\n");
            return range_create_empty();
        }
    }

    uint64_t virt_end = 0;
    if (!range_get_end(virt_range, &virt_end)) {
        printk(LOGLEVEL_WARN, "mmio: map goes beyong end of address-space\n");
        return range_create_empty();
    }
#endif /* !(defined(__riscv) && defined(__LP64__)) */
#endif

    const uint64_t virt_addr = (uint64_t)phys_to_virt(phys_range.front);
    const uint64_t virt_end =
        (uint64_t)phys_to_virt(range_get_end_assert(phys_range));

    if (flags & __VMAP_MMIO_LOW4G) {
        if (virt_end > gib(4)) {
            printk(LOGLEVEL_WARN,
                   "mmio: attempting to map mmio-range that goes beyond 4G in "
                   "lower 4G\n");
            return range_create_empty();
        }
    }

    return range_create_end(virt_addr, virt_end);
}

struct mmio_region *
vmap_mmio(const struct range phys_range,
          const uint8_t prot,
          const uint64_t flags)
{
    if (range_empty(phys_range)) {
        printk(LOGLEVEL_WARN, "mmio: attempting to map empty phys-range\n");
        return NULL;
    }

    if (!range_has_align(phys_range, PAGE_SIZE)) {
        printk(LOGLEVEL_WARN,
               "mmio: phys-range " RANGE_FMT " isn't aligned to the page "
               "size\n",
               RANGE_FMT_ARGS(phys_range));
        return NULL;
    }

    if (prot == PROT_NONE) {
        printk(LOGLEVEL_WARN,
               "mmio: attempting to map mmio range " RANGE_FMT " w/o access "
               "permissions\n",
               RANGE_FMT_ARGS(phys_range));
        return NULL;
    }

    if (prot & PROT_EXEC) {
        printk(LOGLEVEL_WARN,
               "mmio: attempting to map mmio range " RANGE_FMT " with execute "
               "permissions\n",
               RANGE_FMT_ARGS(phys_range));
        return NULL;
    }

    struct mmio_region *const mmio = kmalloc(sizeof(*mmio));
    if (mmio == NULL) {
        printk(LOGLEVEL_WARN,
               "mmio: failed to allocate mmio_region to map phys-range "
               RANGE_FMT "\n",
               RANGE_FMT_ARGS(phys_range));
        return NULL;
    }

    const struct range virt_range = get_virt_range(phys_range, flags);
    if (range_empty(virt_range)) {
        return NULL;
    }

    if (virt_range.front >= gib(4) || flags & __VMAP_MMIO_WT) {
        const bool map_success =
            arch_make_mapping(&kernel_pagemap,
                              phys_range.front,
                              virt_range.front,
                              phys_range.size,
                              prot,
                              VMA_CACHEKIND_WRITETHROUGH,
                              /*is_overwrite=*/true);

        if (!map_success) {
            printk(LOGLEVEL_WARN,
                "mmio: failed to map phys-range " RANGE_FMT " to virtual range "
                RANGE_FMT "\n",
                RANGE_FMT_ARGS(phys_range),
                RANGE_FMT_ARGS(virt_range));
            return NULL;
        }

        mmio->base = (volatile void *)virt_range.front;
        mmio->size = phys_range.size;
    } else {
        mmio->base = (volatile void *)virt_range.front;
        mmio->size = phys_range.size;
    }

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
    kfree(region);

    return true;
}

struct range mmio_region_get_range(const struct mmio_region *const region) {
    return range_create((uint64_t)region->base, region->size);
}