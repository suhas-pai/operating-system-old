/*
 * kernel/mm/mmio.c
 * © suhas pai
 */

#include "dev/printk.h"
#include "mm/page_alloc.h"

#include "kmalloc.h"
#include "mmio.h"
#include "pagemap.h"

static struct address_space mmio_space = ADDRSPACE_INIT(mmio_space);
static struct spinlock lock = {};

enum mmio_region_flags {
    MMIO_REGION_LOW4G = 1 << 0
};

enum prot_fail {
    PROT_FAIL_NONE,
    PROT_FAIL_PROT_NONE,
    PROT_FAIL_PROT_EXEC
};

static enum prot_fail verify_prot(const uint8_t prot) {
    if (prot == PROT_NONE) {
        return PROT_FAIL_PROT_NONE;
    }

    if (prot & PROT_EXEC) {
        return PROT_FAIL_PROT_EXEC;
    }

    return PROT_FAIL_NONE;
}

// 16kib of guard pages
#define GUARD_PAGE_SIZE (PAGE_SIZE * 4)

struct mmio_region *
map_mmio_region(const struct range phys_range,
                const uint8_t prot,
                const uint64_t flags)
{
    struct range in_range = range_create(VMAP_BASE, (VMAP_END - VMAP_BASE));
    struct mmio_region *const mmio = kmalloc(sizeof(*mmio));

    if (mmio == NULL) {
        printk(LOGLEVEL_WARN,
               "vmap_mmio(): failed to allocate mmio_region to map phys-range "
               RANGE_FMT "\n",
               RANGE_FMT_ARGS(phys_range));
        return NULL;
    }

    addrspace_node_init(&mmio_space, &mmio->node);
    mmio->node.range.size = phys_range.size + GUARD_PAGE_SIZE;

    const int flag = spin_acquire_with_irq(&lock);
    const uint64_t virt_addr =
        addrspace_find_space_for_node(&mmio_space,
                                      in_range,
                                      &mmio->node,
                                      /*align=*/PAGE_SIZE);

    if (virt_addr == ADDRSPACE_INVALID_ADDR) {
        spin_release_with_irq(&lock, flag);
        printk(LOGLEVEL_WARN,
               "vmap_mmio(): failed to find suitable virtual-address range to "
               "map phys-range " RANGE_FMT "\n",
               RANGE_FMT_ARGS(phys_range));

        return NULL;
    }

    const struct range virt_range = range_create(virt_addr, phys_range.size);
    const bool map_success =
        arch_make_mapping(&kernel_pagemap,
                          phys_range.front,
                          virt_range.front,
                          phys_range.size,
                          prot,
                          (flags & __VMAP_MMIO_WT) ?
                            VMA_CACHEKIND_WRITETHROUGH : VMA_CACHEKIND_MMIO,
                          /*is_overwrite=*/true);

    spin_release_with_irq(&lock, flag);
    if (!map_success) {
        printk(LOGLEVEL_WARN,
               "vmap_mmio(): failed to map phys-range " RANGE_FMT " to virtual "
               "range " RANGE_FMT "\n",
               RANGE_FMT_ARGS(phys_range),
               RANGE_FMT_ARGS(virt_range));

        kfree(mmio);
        return NULL;
    }

    mmio->base = (volatile void *)virt_range.front;
    mmio->size = phys_range.size;

    return mmio;
}

struct mmio_region *
vmap_mmio_low4g(const uint8_t prot, const uint64_t order, const uint64_t flags)
{
    if (order == 0) {
        printk(LOGLEVEL_WARN,
               "vmap_mmio_low4g(): attempting to map an empty low-4g range\n");
        return NULL;
    }

    switch (verify_prot(prot)) {
        case PROT_FAIL_NONE:
            break;
        case PROT_FAIL_PROT_NONE:
            printk(LOGLEVEL_WARN,
                   "vmap_mmio_low4g(): attempting to map a low-4g mmio range "
                   "w/o access permissions\n");
            return NULL;
        case PROT_FAIL_PROT_EXEC:
            printk(LOGLEVEL_WARN,
                   "vmap_mmio_low4g(): attempting to map a low-4g mmio range "
                   "with execute permissions\n");
            return NULL;
    }

    struct page *const page = alloc_pages(__ALLOC_ZERO | __ALLOC_LOW4G, order);
    if (page == NULL) {
        printk(LOGLEVEL_WARN,
               "vmap_mmio_low4g(): failed to allocate low-4g pages to map a "
               "mmio range\n");
        return NULL;
    }

    const struct range phys_range =
        range_create(page_to_phys(page), PAGE_SIZE * order);

    struct mmio_region *const mmio = map_mmio_region(phys_range, prot, flags);
    if (mmio == NULL) {
        free_pages(page, order);
        return NULL;
    }

    return mmio;
}

struct mmio_region *
vmap_mmio(const struct range phys_range,
          const uint8_t prot,
          const uint64_t flags)
{
    if (range_empty(phys_range)) {
        printk(LOGLEVEL_WARN,
               "vmap_mmio(): attempting to map empty phys-range\n");
        return NULL;
    }

    if (!range_has_align(phys_range, PAGE_SIZE)) {
        printk(LOGLEVEL_WARN,
               "vmap_mmio(): phys-range " RANGE_FMT " isn't aligned to the "
               "page size\n",
               RANGE_FMT_ARGS(phys_range));
        return NULL;
    }

    switch (verify_prot(prot)) {
        case PROT_FAIL_NONE:
            break;
        case PROT_FAIL_PROT_NONE:
            printk(LOGLEVEL_WARN,
                   "vmap_mmio(): attempting to map mmio range " RANGE_FMT
                   " w/o access permissions\n",
                   RANGE_FMT_ARGS(phys_range));
            return NULL;
        case PROT_FAIL_PROT_EXEC:
            printk(LOGLEVEL_WARN,
                   "vmap_mmio(): attempting to map mmio range " RANGE_FMT
                   " with execute permissions\n",
                   RANGE_FMT_ARGS(phys_range));
            return NULL;
    }

    return map_mmio_region(phys_range, prot, flags);
}

bool vunmap_mmio(struct mmio_region *const region) {
    const int flag = spin_acquire_with_irq(&kernel_pagemap.addrspace_lock);
    addrspace_remove_node(&region->node);

    uint64_t phys = 0;
    if (region->flags & MMIO_REGION_LOW4G) {
        phys = ptwalker_virt_get_phys(&kernel_pagemap, (uint64_t)region->base);
    }

    if (phys == INVALID_PHYS) {
        spin_release_with_irq(&kernel_pagemap.addrspace_lock, flag);
        return false;
    }

    arch_unmap_mapping(&kernel_pagemap, (uint64_t)region->base, region->size);
    spin_release_with_irq(&kernel_pagemap.addrspace_lock, flag);

    if (region->flags & MMIO_REGION_LOW4G) {
        free_pages(phys_to_page(phys), /*order=*/region->size / PAGE_SIZE);
    }

    kfree(region);
    return true;
}

struct range mmio_region_get_range(const struct mmio_region *const region) {
    return range_create((uint64_t)region->base, region->size);
}