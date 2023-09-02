/*
 * kernel/arch/aarch64/mm/vma.c
 * © suhas pai
 */

#include "lib/align.h"

#include "mm/pagemap.h"
#include "mm/pageop.h"

static inline uint64_t
flags_from_info(const uint8_t prot, const enum vma_cachekind cachekind) {
    uint64_t result =
        __PTE_VALID | __PTE_INNER_SH | __PTE_ACCESS | __PTE_4KPAGE;

    if (!(prot & PROT_WRITE)) {
        result |= __PTE_RO;
    }

    if (!(prot & PROT_EXEC)) {
        result |= __PTE_UXN | __PTE_PXN;
    }

    switch (cachekind) {
        case VMA_CACHEKIND_WRITEBACK:
            break;
        case VMA_CACHEKIND_WRITETHROUGH:
            result |= __PTE_WT;
            break;
        case VMA_CACHEKIND_WRITECOMBINING:
            result |= __PTE_WC;
            break;
        case VMA_CACHEKIND_NO_CACHE:
            result |= __PTE_UNPREDICT;
            break;
        case VMA_CACHEKIND_MMIO:
            result |= __PTE_MMIO;
            break;
    }

    return result;
}

static bool
undo_changes(struct pt_walker *const walker,
             struct pageop *const pageop,
             const uint64_t size)
{
    for (uint64_t i = 0; i != size; i += PAGE_SIZE) {
        walker->tables[0][walker->indices[0]] = 0;

        ptwalker_deref_from_level(walker, /*level=*/1, pageop);
        const enum pt_walker_result ptwalker_result =
            ptwalker_prev(walker, pageop);

        if (ptwalker_result != E_PT_WALKER_OK) {
            pageop_finish(pageop);
            return false;
        }
    }

    return true;
}

bool
arch_make_mapping(struct pagemap *const pagemap,
                  const uint64_t phys_addr,
                  const uint64_t virt_addr,
                  const uint64_t size,
                  const uint8_t prot,
                  const enum vma_cachekind cachekind,
                  const bool is_overwrite)
{
    assert(has_align(size, PAGE_SIZE));

    // TODO: Add Huge page support
    struct pageop pageop;
    pageop_init(&pageop);

    const int flag = spin_acquire_with_irq(&pagemap->addrspace_lock);

    struct pt_walker walker;
    ptwalker_default_for_pagemap(&walker, pagemap, virt_addr);

    enum pt_walker_result ptwalker_result =
        ptwalker_fill_in_to(&walker,
                            /*level=*/1,
                            /*should_ref=*/true,
                            /*alloc_pgtable_cb_info=*/NULL,
                            /*free_pgtable_cb_info=*/&pageop);

    if (ptwalker_result != E_PT_WALKER_OK) {
        pageop_finish(&pageop);
        spin_release_with_irq(&pagemap->addrspace_lock, flag);

        return false;
    }

    const uint64_t flags = flags_from_info(prot, cachekind);
    if (is_overwrite) {
        for (uint64_t i = 0; i != size; i += PAGE_SIZE) {
            const pte_t entry = walker.tables[0][walker.indices[0]];
            const pte_t new_entry = phys_create_pte(phys_addr + i) | flags;

            walker.tables[0][walker.indices[0]] = new_entry;
            if (pte_is_present(entry)) {
                const uint64_t flags_mask =
                    __PTE_VALID | __PTE_TABLE | __PTE_USER | __PTE_RO |
                    __PTE_INNER_SH | __PTE_ACCESS | __PTE_PXN | __PTE_UXN;

                if ((entry & flags_mask) != (new_entry & flags_mask) ||
                    pte_to_phys(entry) != pte_to_phys(new_entry))
                {
                    //pageop_flush(&pageop, virt_addr + i);
                }
            }

            ptwalker_result = ptwalker_next(&walker, &pageop);
            if (ptwalker_result != E_PT_WALKER_OK) {
                pageop_finish(&pageop);
                spin_release_with_irq(&pagemap->addrspace_lock, flag);

                return false;
            }
        }
    } else {
        for (uint64_t i = 0; i != size; i += PAGE_SIZE) {
            walker.tables[0][walker.indices[0]] =
                phys_create_pte(phys_addr + i) | flags;

            ptwalker_result = ptwalker_next(&walker, &pageop);
            if (ptwalker_result != E_PT_WALKER_OK) {
                undo_changes(&walker, &pageop, i);
                pageop_finish(&pageop);
                spin_release_with_irq(&pagemap->addrspace_lock, flag);

                return false;
            }
        }
    }

    pageop_finish(&pageop);
    spin_release_with_irq(&pagemap->addrspace_lock, flag);

    return true;
}

bool
arch_unmap_mapping(struct pagemap *pagemap,
                   uint64_t virt_addr,
                   uint64_t size)
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
            pageop_finish(&pageop);
            spin_release_with_irq(&pagemap->addrspace_lock, flag);

            return false;
        }
    }

    pageop_finish(&pageop);
    spin_release_with_irq(&pagemap->addrspace_lock, flag);

    return true;
}