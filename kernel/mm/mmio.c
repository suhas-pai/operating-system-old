/*
 * kernel/mm/mmio.c
 * © suhas pai
 */

#include "dev/printk.h"
#include "lib/size.h"

#include "kmalloc.h"
#include "mmio.h"
#include "pagemap.h"

static struct address_space mmio_space = ADDRSPACE_INIT(mmio_space);
static struct spinlock lock = {};

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

    struct range in_range = range_create(MMIO_BASE, (MMIO_END - MMIO_BASE));
    struct mmio_region *const mmio = kmalloc(sizeof(*mmio));

    if (mmio == NULL) {
        printk(LOGLEVEL_WARN,
               "mmio: failed to allocate mmio_region to map phys-range "
               RANGE_FMT "\n",
               RANGE_FMT_ARGS(phys_range));
        return NULL;
    }

    addrspace_node_init(&mmio_space, &mmio->node);
    mmio->node.range.size = phys_range.size;

    const int flag = spin_acquire_with_irq(&lock);
    const uint64_t virt_addr =
        addrspace_find_space_for_node(&mmio_space,
                                      in_range,
                                      &mmio->node,
                                      /*align=*/PAGE_SIZE);

    if (virt_addr == ADDRSPACE_INVALID_ADDR) {
        spin_release_with_irq(&lock, flag);
        printk(LOGLEVEL_WARN,
               "mmio: failed to find suitable virtual-address range to map "
               "phys-range " RANGE_FMT "\n",
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
                            VMA_CACHEKIND_WRITETHROUGH :
                            VMA_CACHEKIND_MMIO,
                            /*is_overwrite=*/true);

    spin_release_with_irq(&lock, flag);
    if (!map_success) {
        printk(LOGLEVEL_WARN,
                "mmio: failed to map phys-range " RANGE_FMT " to virtual "
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

bool vunmap_mmio(struct mmio_region *const region) {
    addrspace_remove_node(&region->node);
    const int flag = spin_acquire_with_irq(&kernel_pagemap.lock);

    arch_unmap_mapping(&kernel_pagemap, (uint64_t)region->base, region->size);
    spin_release_with_irq(&kernel_pagemap.lock, flag);
    kfree(region);

    return true;
}

struct range mmio_region_get_range(const struct mmio_region *const region) {
    return range_create((uint64_t)region->base, region->size);
}