/*
 * kernel/mm/walker.c
 * © suhas pai
 */

#if defined(__x86_64__)
    #include "asm/regs.h"
#elif defined(__aarch64__)
    #include "asm/ttbr.h"
#endif /* defined(__x86_64__) */

#include "lib/align.h"

#include "mm_types.h"
#include "pagemap.h"

#include "walker.h"

static struct page *
ptwalker_alloc_pgtable_cb(struct pt_walker *const walker, void *const cb_info) {
    (void)walker;
    (void)cb_info;

    return alloc_table();
}

static void
ptwalker_free_pgtable_cb(struct pt_walker *const walker,
                         struct page *const page,
                         void *const cb_info)
{
    (void)walker;
    pageop_free_table((struct pageop *)cb_info, page);
}

static inline uint64_t
get_root_phys(const struct pagemap *const pagemap, const uint64_t virt_addr) {
#if defined(__aarch64__)
    uint64_t root_phys = 0;
    if (virt_addr & (1ull << 63)) {
        root_phys = page_to_phys(pagemap->root[1]);
    } else {
        root_phys = page_to_phys(pagemap->root[0]);
    }
#else
    (void)virt_addr;
    const uint64_t root_phys = page_to_phys(pagemap->root);
#endif /* defined(__x86_64__) */

    return root_phys;
}

void
ptwalker_default(struct pt_walker *const walker, const uint64_t virt_addr) {
    return ptwalker_default_for_pagemap(walker, &kernel_pagemap, virt_addr);
}

void
ptwalker_default_for_pagemap(struct pt_walker *const walker,
                             struct pagemap *const pagemap,
                             const uint64_t virt_addr)
{
    return ptwalker_create_customroot(walker,
                                      get_root_phys(pagemap, virt_addr),
                                      virt_addr,
                                      ptwalker_alloc_pgtable_cb,
                                      ptwalker_free_pgtable_cb);
}

void
ptwalker_create(struct pt_walker *const walker,
                const uint64_t virt_addr,
                const ptwalker_alloc_pgtable_t alloc_pgtable,
                const ptwalker_free_pgtable_t free_pgtable)
{
    return ptwalker_create_customroot(walker,
                                      get_root_phys(&kernel_pagemap, virt_addr),
                                      virt_addr,
                                      alloc_pgtable,
                                      free_pgtable);
}

void
ptwalker_create_customroot(struct pt_walker *const walker,
                           const uint64_t root_phys,
                           const uint64_t virt_addr,
                           const ptwalker_alloc_pgtable_t alloc_pgtable,
                           const ptwalker_free_pgtable_t free_pgtable)
{
    assert(has_align(root_phys, PAGE_SIZE));
    assert(has_align(virt_addr, PAGE_SIZE));

    walker->level = pgt_get_top_level();
    walker->top_level = walker->level;

    walker->alloc_pgtable = alloc_pgtable;
    walker->free_pgtable = free_pgtable;

    walker->tables[walker->top_level - 1] = phys_to_virt(root_phys);
    walker->indices[walker->top_level - 1] =
        virt_to_pt_index((void *)virt_addr, walker->top_level);

    pte_t *prev_table = walker->tables[walker->top_level - 1];
    for (uint8_t level = walker->top_level - 1; level >= 1; level--) {
        pte_t *table = NULL;
        if (prev_table != NULL) {
            const uint8_t parent_level = level + 1;
            const uint16_t index = walker->indices[parent_level - 1];

            const pte_t entry = prev_table[index];
            if (pte_is_present(entry)) {
                if (!pte_level_can_have_large(parent_level) ||
                    !pte_is_large(entry))
                {
                    table = pte_to_virt(entry);
                    walker->level = level;
                } else {
                    walker->level = parent_level;
                }
            }
        }

        walker->tables[level - 1] = table;
        walker->indices[level - 1] = virt_to_pt_index((void *)virt_addr, level);

        prev_table = table;
    }

    for (uint8_t index = walker->top_level; index != PGT_LEVEL_COUNT; index++) {
        walker->tables[index] = NULL;
        walker->indices[index] = UINT16_MAX;
    }
}

pte_t *
ptwalker_table_for_level(const struct pt_walker *const walker,
                         const uint8_t level)
{
    return walker->tables[level - 1];
}

uint16_t
ptwalker_table_index(const struct pt_walker *const walker, const uint8_t level)
{
    return walker->indices[level - 1];
}

pte_t *
ptwalker_pte_in_level(const struct pt_walker *const walker, const uint8_t level)
{
    pte_t *const table = ptwalker_table_for_level(walker, level);
    if (table != NULL) {
        return table + ptwalker_table_index(walker, level);
    }

    return NULL;
}

enum pt_walker_result
ptwalker_next(struct pt_walker *const walker, struct pageop *const pageop) {
    return ptwalker_next_with_options(walker,
                                      /*level=*/walker->level,
                                      /*alloc_parents=*/true,
                                      /*alloc_level=*/true,
                                      /*should_ref=*/true,
                                      /*alloc_pgtable_cb_info=*/NULL,
                                      /*free_pgtable_cb_info=*/pageop);
}

static void
reset_levels_lower_than(struct pt_walker *const walker, uint8_t level) {
    for (level--; level >= 1; level--) {
        walker->tables[level - 1] = NULL;
        walker->indices[level - 1] = 0;
    }
}

static void
setup_levels_lower_than(struct pt_walker *const walker,
                        const uint8_t lower_than_level,
                        pte_t *const first_pte)
{
    pte_t *pte = first_pte;
    uint8_t level = lower_than_level - 1;

    while (true) {
        pte_t *const table = pte_to_virt(*pte);

        walker->tables[level - 1] = table;
        walker->indices[level - 1] = 0;

        pte = table;
        level--;

        if (level < 1) {
            walker->level = 1;
            break;
        }

        if (!pte_is_present(*pte) ||
            (pte_level_can_have_large(level) && pte_is_large(*pte)))
        {
            walker->level = level;
            break;
        }
    }
}

static void
ptwalker_drop_lowest(struct pt_walker *const walker,
                     void *const free_pgtable_cb_info)
{
    walker->tables[walker->level - 1] = NULL;
    walker->indices[walker->level - 1] = UINT16_MAX;

    ptwalker_deref_from_level(walker, walker->level + 1, free_pgtable_cb_info);
}

static inline bool
alloc_single_pte(struct pt_walker *const walker,
                 void *const alloc_pgtable_cb_info,
                 void *const free_pgtable_cb_info,
                 const uint8_t level,
                 pte_t *const pte)
{
    struct page *const pt =
        walker->alloc_pgtable(walker, alloc_pgtable_cb_info);

    if (pt == NULL) {
        walker->level = level + 1;
        ptwalker_drop_lowest(walker, free_pgtable_cb_info);

        return false;
    }

    walker->tables[level - 1] = page_to_virt(pt);
    *pte = phys_create_pte(page_to_phys(pt)) | PGT_FLAGS;

    return true;
}

static enum pt_walker_result
alloc_levels_down_to(struct pt_walker *const walker,
                     uint8_t parent_level,
                     uint8_t last_level,
                     const bool alloc_last_level,
                     const bool should_ref,
                     void *const alloc_pgtable_cb_info,
                     void *const free_pgtable_cb_info)
{
    const ptwalker_alloc_pgtable_t alloc_pgtable = walker->alloc_pgtable;
    assert(alloc_pgtable != NULL);

    uint8_t level = parent_level - 1;
    if (level == last_level) {
        if (!alloc_last_level) {
            walker->level = level + 1;
            return E_PT_WALKER_OK;
        }
    }

    do {
        pte_t *const pte = ptwalker_pte_in_level(walker, level + 1);
        if (!alloc_single_pte(walker,
                              alloc_pgtable_cb_info,
                              free_pgtable_cb_info,
                              level,
                              pte))
        {
            walker->level = level + 1;
            ptwalker_drop_lowest(walker, free_pgtable_cb_info);

            return E_PT_WALKER_ALLOC_FAIL;
        }

        if (should_ref) {
            pte_t *const table = ptwalker_table_for_level(walker, level + 1);
            ref_up(&virt_to_page(table)->table.refcount);
        }

        level--;
        if (level == last_level) {
            if (!alloc_last_level) {
                break;
            }
        } else if (level < last_level) {
            break;
        }
    } while (true);

    walker->level = level + 1;
    return E_PT_WALKER_OK;
}

enum pt_walker_result
ptwalker_next_with_options(struct pt_walker *const walker,
                           uint8_t level,
                           const bool alloc_parents,
                           const bool alloc_level,
                           const bool should_ref,
                           void *const alloc_pgtable_cb_info,
                           void *const free_pgtable_cb_info)
{
    // Bad increment as tables+indices haven't been filled down to the level
    // requested.

    if (walker->level > level || level > walker->top_level) {
        return E_PT_WALKER_BAD_INCR;
    }

    if (walker->level < level) {
        reset_levels_lower_than(walker, level);
    }

    walker->indices[level - 1]++;
    if (walker->indices[level - 1] < PGT_COUNT) {
        if (level == 1 || !pte_level_can_have_large(level)) {
            return E_PT_WALKER_OK;
        }

        pte_t *const pte = ptwalker_pte_in_level(walker, level);
        const pte_t entry = *pte;

        if (!pte_is_present(entry) || pte_is_large(entry)) {
            return E_PT_WALKER_OK;
        }

        setup_levels_lower_than(walker, level, pte);
        return E_PT_WALKER_OK;
    }

    const uint8_t orig_level = level;
    do {
        level++;
        if (level > walker->top_level) {
            // Reset all of walker, except the root pointer
            walker->indices[level - 1] = 0;
            walker->level = PTWALKER_DONE;

            reset_levels_lower_than(walker, level);
            return E_PT_WALKER_REACHED_END;
        }

        walker->indices[level - 1]++;
        if (walker->indices[level - 1] == PGT_COUNT) {
            continue;
        }

        if (!pte_level_can_have_large(level)) {
            reset_levels_lower_than(walker, level);
            break;
        }

        pte_t *const pte =
            &walker->tables[level - 1][walker->indices[level - 1]];

        const pte_t entry = *pte;
        if (!pte_is_present(entry)) {
            reset_levels_lower_than(walker, level);
            break;
        }

        if (pte_is_large(entry)) {
            reset_levels_lower_than(walker, level);
            walker->level = level;

            return E_PT_WALKER_OK;
        }

        setup_levels_lower_than(walker, level, pte);
        return E_PT_WALKER_OK;
    } while (true);

    if (!alloc_parents) {
        walker->level = level;
        return E_PT_WALKER_OK;
    }

    return alloc_levels_down_to(walker,
                                level,
                                orig_level,
                                alloc_level,
                                should_ref,
                                alloc_pgtable_cb_info,
                                free_pgtable_cb_info);
}

enum pt_walker_result
ptwalker_prev(struct pt_walker *const walker, struct pageop *const pageop) {
    return ptwalker_prev_custom(walker,
                                /*level=*/1,
                                /*alloc_parents=*/true,
                                /*alloc_level=*/true,
                                /*should_ref=*/true,
                                /*alloc_pgtable_cb_info=*/NULL,
                                /*free_pgtable_cb_info=*/pageop);
}

enum pt_walker_result
ptwalker_prev_custom(struct pt_walker *const walker,
                     uint8_t level,
                     const bool alloc_parents,
                     const bool alloc_level,
                     const bool should_ref,
                     void *const alloc_pgtable_cb_info,
                     void *const free_pgtable_cb_info)
{
    // Bad increment as tables+indices haven't been filled down to the level
    // requested.

    if (walker->level > level || level > walker->top_level) {
        return E_PT_WALKER_BAD_INCR;
    }

    if (walker->level < level) {
        reset_levels_lower_than(walker, level);
    }

    if (walker->indices[level - 1] != 0) {
        walker->indices[level - 1]--;
        if (level == 1 || !pte_level_can_have_large(level)) {
            return E_PT_WALKER_OK;
        }

        pte_t *const pte = ptwalker_pte_in_level(walker, level);
        const pte_t entry = *pte;

        if (!pte_is_present(entry) || pte_is_large(entry)) {
            return E_PT_WALKER_OK;
        }

        setup_levels_lower_than(walker, level, pte);
        return E_PT_WALKER_OK;
    }

    walker->indices[level - 1] = PGT_COUNT - 1;
    const uint8_t orig_level = level;

    do {
        level++;
        if (level > walker->top_level) {
            // Reset all of walker, except the root pointer
            walker->indices[level - 1] = 0;
            walker->level = PTWALKER_DONE;

            reset_levels_lower_than(walker, level);
            return E_PT_WALKER_REACHED_END;
        }

        if (walker->indices[level - 1] == 0) {
            walker->indices[level - 1] = PGT_COUNT - 1;
            continue;
        }

        walker->indices[level - 1]--;
        if (!pte_level_can_have_large(level)) {
            reset_levels_lower_than(walker, level);
            break;
        }

        pte_t *const pte =
            &walker->tables[level - 1][walker->indices[level - 1]];

        const pte_t entry = *pte;
        if (!pte_is_present(entry)) {
            reset_levels_lower_than(walker, level);
            break;
        }

        if (pte_is_large(entry)) {
            reset_levels_lower_than(walker, level);
            walker->level = level;

            return E_PT_WALKER_OK;
        }

        setup_levels_lower_than(walker, level, pte);
        return E_PT_WALKER_OK;
    } while (true);

    if (!alloc_parents) {
        walker->level = level;
        return E_PT_WALKER_OK;
    }

    return alloc_levels_down_to(walker,
                                level,
                                orig_level,
                                alloc_level,
                                should_ref,
                                alloc_pgtable_cb_info,
                                free_pgtable_cb_info);
}

void
ptwalker_fill_in_lowest(struct pt_walker *const walker, struct page *const page)
{
    if (walker->level == 1) {
        return;
    }

    const uint8_t level = walker->level - 1;

    pte_t *const table = ptwalker_table_for_level(walker, level + 1);
    pte_t *const pte = ptwalker_pte_in_level(walker, level + 1);

    *pte = page_to_phys(page) | PGT_FLAGS;

    ref_up(&virt_to_page(table)->table.refcount);
    ref_up(&page->table.refcount);
}

enum pt_walker_result
ptwalker_fill_in_to(struct pt_walker *const walker,
                    const uint8_t level,
                    const bool should_ref,
                    void *const alloc_pgtable_cb_info,
                    void *const free_pgtable_cb_info)
{
    if (walker->level <= level) {
        return E_PT_WALKER_OK;
    }

    return alloc_levels_down_to(walker,
                                walker->level,
                                level,
                                /*alloc_level=*/true,
                                should_ref,
                                alloc_pgtable_cb_info,
                                free_pgtable_cb_info);
}

void
ptwalker_deref_from_level(struct pt_walker *const walker,
                          uint8_t level,
                          void *const free_pgtable_cb_info)
{
    const ptwalker_free_pgtable_t free_pgtable = walker->free_pgtable;
    assert(free_pgtable != NULL);

    for (; level <= walker->top_level; level++) {
        pte_t *const table = walker->tables[level - 1];
        struct page *const pt = virt_to_page(table);

        if (!ref_down(&pt->table.refcount)) {
            break;
        }

        free_pgtable(walker, pt, free_pgtable_cb_info);
    }

    walker->level = level;
}