/*
 * kernel/arch/aarch64/mm/vma.c
 * © suhas pai
 */

#include "mm/pagemap.h"

static inline uint64_t
flags_from_info(const uint8_t prot, const enum vma_cachekind cachekind) {
    uint64_t result = __PTE_VALID | __PTE_INNER_SHARE | __PTE_ACCESS;
    if (!(prot & PROT_WRITE)) {
        result |= __PTE_RO;
    }

    if (!(prot & PROT_EXEC)) {
        result |= __PTE_NOEXEC;
    }

    // TODO:
    (void)cachekind;
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
    // TODO: Add Huge page support
    struct pageop pageop;
    pageop_init(&pageop);

    struct pt_walker walker;
    ptwalker_default_with_root(&walker, page_to_phys(pagemap->root), virt_addr);

    enum pt_walker_result ptwalker_result =
        ptwalker_fill_in_to(&walker,
                            /*level=*/1,
                            /*should_ref=*/true,
                            /*alloc_pgtable_cb_info=*/NULL,
                            /*free_pgtable_cb_info=*/&pageop);

    if (ptwalker_result != E_PT_WALKER_OK) {
        pageop_finish(&pageop);
        return false;
    }

    const uint64_t flags = flags_from_info(prot, cachekind);
    if (is_overwrite) {
        for (uint64_t i = 0; i != size; i += PAGE_SIZE) {
            const pte_t entry = walker.tables[0][walker.indices[0]];
            const pte_t new_entry = (phys_addr + i) | flags;

            walker.tables[0][walker.indices[0]] = new_entry;
            if (pte_is_present(entry)) {
                const uint64_t flags_mask =
                    __PTE_VALID | __PTE_TABLE | __PTE_USER | __PTE_RO |
                    __PTE_INNER_SHARE | __PTE_ACCESS | __PTE_NOEXEC;

                if ((entry & flags_mask) != (new_entry & flags_mask) ||
                    (entry & PTE_PHYS_MASK) != (new_entry & PTE_PHYS_MASK))
                {
                    pageop_flush(&pageop, virt_addr + i);
                }
            }

            ptwalker_result = ptwalker_next(&walker, &pageop);
            if (ptwalker_result != E_PT_WALKER_OK) {
                undo_changes(&walker, &pageop, i);
                pageop_finish(&pageop);

                return false;
            }
        }
    } else {
        for (uint64_t i = 0; i != size; i += PAGE_SIZE) {
            walker.tables[0][walker.indices[0]] = (phys_addr + i) | flags;
            ptwalker_result = ptwalker_next(&walker, &pageop);

            if (ptwalker_result != E_PT_WALKER_OK) {
                undo_changes(&walker, &pageop, i);
                pageop_finish(&pageop);

                return false;
            }
        }
    }

    pageop_finish(&pageop);
    return true;
}