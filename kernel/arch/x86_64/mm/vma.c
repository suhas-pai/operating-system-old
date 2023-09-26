/*
 * kernel/arch/x86_64/mm/vma.c
 * © suhas pai
 */

#include "mm/pagemap.h"
#include "mm/pageop.h"
#include "mm/pgmap.h"
#include "mm/walker.h"

#include "cpu.h"

static inline uint64_t
flags_from_info(struct pagemap *const pagemap,
                const uint8_t prot,
                const enum vma_cachekind cachekind)
{
    uint64_t result = __PTE_PRESENT;
    if (pagemap == &kernel_pagemap) {
        result |= __PTE_GLOBAL;
    } else {
        result |= __PTE_USER;
    }

    if (prot & PROT_WRITE) {
        result |= __PTE_WRITE;
    }

    if ((prot & PROT_EXEC) == 0) {
        result |= __PTE_NOEXEC;
    }

    switch (cachekind) {
        case VMA_CACHEKIND_WRITEBACK:
            break;
        case VMA_CACHEKIND_WRITETHROUGH:
            result |= __PTE_PWT;
            break;
        case VMA_CACHEKIND_WRITECOMBINING:
            result |= __PTE_WC;
            break;
        case VMA_CACHEKIND_NO_CACHE:
            result |= __PTE_PCD;
            break;
    }

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
    const uint64_t pte_flags = flags_from_info(pagemap, prot, cachekind);
    const bool supports_large = (pte_flags & __PTE_WC) == 0;

    const struct pgmap_options options = {
        .pte_flags = pte_flags,

        .alloc_pgtable_cb_info = NULL,
        .free_pgtable_cb_info = NULL,

        .supports_largepage_at_level_mask =
            (supports_large << 2 |
             (supports_large &&
                get_cpu_capabilities()->supports_1gib_pages) << 3),

        .is_in_early = false,
        .free_pages = false,
        .is_overwrite = is_overwrite
    };

    pgmap_at(pagemap, phys_range, virt_addr, &options);
    return true;
}

bool
arch_unmap_mapping(struct pagemap *const pagemap,
                   const uint64_t virt_addr,
                   const uint64_t size)
{
    struct pageop pageop;
    pageop_init(&pageop);

    struct pt_walker walker;
    ptwalker_default_for_pagemap(&walker, pagemap, virt_addr);

    for (uint64_t i = 0; i != size; i += PAGE_SIZE) {
        pte_t *const pte = walker.tables[0] + walker.indices[0];
        const pte_t entry = pte_read(pte);

        pte_write(pte, /*value=*/0);
        if (pte_is_present(entry)) {
            pageop_flush_address(&pageop, virt_addr + i);
        }

        const enum pt_walker_result result = ptwalker_next(&walker);
        if (result != E_PT_WALKER_OK) {
            pageop_finish(&pageop);
            return false;
        }
    }

    pageop_finish(&pageop);
    return true;
}