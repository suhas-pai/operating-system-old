/*
 * kernel/arch/riscv64/mm/vma.c
 * © suhas pai
 */

#include "lib/align.h"

#include "mm/pagemap.h"
#include "mm/pageop.h"
#include "mm/pgmap.h"

static inline uint64_t
flags_from_info(const uint8_t prot, const enum vma_cachekind cachekind) {
    const uint64_t result =
        __PTE_VALID | __PTE_ACCESSED | __PTE_DIRTY | (uint64_t)(prot << 1);

    // TODO:
    (void)cachekind;
    return result;
}

bool
arch_make_mapping(struct pagemap *const pagemap,
                  const struct range phys_range,
                  const uint64_t virt_addr,
                  const uint8_t prot,
                  const enum vma_cachekind cachekind,
                  const bool is_overwrite)
{
    const int flag = spin_acquire_with_irq(&pagemap->addrspace_lock);
    const struct pgmap_options options = {
        .pte_flags = flags_from_info(prot, cachekind),

        .alloc_pgtable_cb_info = NULL,
        .free_pgtable_cb_info = NULL,

        .supports_largepage_at_level_mask = 1 << 2 | 1 << 3,

        .is_in_early = false,
        .is_overwrite = is_overwrite
    };

    pgmap_at(pagemap, phys_range, virt_addr, &options);
    spin_release_with_irq(&pagemap->addrspace_lock, flag);

    return true;
}

bool
arch_unmap_mapping(struct pagemap *const pagemap,
                   const uint64_t virt_addr,
                   const uint64_t size)
{
    struct pageop pageop;
    pageop_init(&pageop);

    const int flag = spin_acquire_with_irq(&pagemap->addrspace_lock);

    struct pt_walker walker;
    ptwalker_default_for_pagemap(&walker, pagemap, virt_addr);

    for (uint64_t i = 0; i != size; i += PAGE_SIZE) {
        const pte_t entry = walker.tables[0][walker.indices[0]];
        walker.tables[0][walker.indices[0]] = 0;

        if (pte_is_present(entry)) {
            //pageop_flush(&pageop, virt_addr + i);
        }

        const enum pt_walker_result result = ptwalker_next(&walker, &pageop);
        if (result != E_PT_WALKER_OK) {
            //pageop_finish(&pageop);
            spin_release_with_irq(&pagemap->addrspace_lock, flag);

            return false;
        }
    }

    pageop_finish(&pageop);
    spin_release_with_irq(&pagemap->addrspace_lock, flag);

    return true;
}