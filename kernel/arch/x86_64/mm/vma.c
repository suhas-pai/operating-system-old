/*
 * kernel/arch/x86_64/mm/vma.c
 * © suhas pai
 */

#include "lib/align.h"

#include "mm/pagemap.h"
#include "mm/walker.h"

static inline uint64_t
flags_from_info(const uint8_t prot, const enum vma_cachekind cachekind) {
    uint64_t result = __PTE_PRESENT;
    if (prot & PROT_WRITE) {
        result |= __PTE_WRITE;
    }

    if (!(prot & PROT_EXEC)) {
        result |= __PTE_NOEXEC;
    }

    switch (cachekind) {
        case VMA_CACHEKIND_WRITEBACK:
            break;
        case VMA_CACHEKIND_WRITETHROUGH:
            result |= __PTE_PWT;
            break;
        case VMA_CACHEKIND_WRITECOMBINING:
            result |= __PTE_PAT;
            break;
        case VMA_CACHEKIND_NO_CACHE:
            result |= __PTE_PCD;
            break;
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
                  const bool needs_flush)
{
    assert(has_align(size, PAGE_SIZE));

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
    for (uint64_t i = 0; i != size; i += PAGE_SIZE) {
        walker.tables[0][walker.indices[0]] = (phys_addr + i) | flags;
        ptwalker_result = ptwalker_next(&walker, &pageop);

        if (ptwalker_result != E_PT_WALKER_OK) {
            undo_changes(&walker, &pageop, i);
            pageop_finish(&pageop);

            return false;
        }
    }

    if (needs_flush) {
        pageop_flush_range(&pageop, range_create(virt_addr, size));
    }

    pageop_finish(&pageop);
    return true;
}
