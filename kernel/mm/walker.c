/*
 * kernel/mm/walker.c
 * © suhas pai
 */

#if defined(__x86_64__)
    #include "arch/x86_64/asm/regs.h"
#endif /* defined(__x86_64__) */

#include "lib/align.h"
#include "mm/pagemap.h"

#include "limine.h"
#include "walker.h"

void
ptwalker_default(struct pt_walker *const walker,
                 const uint64_t virt_addr,
                 const ptwalker_alloc_pgtable_t alloc_pgtable,
                 const ptwalker_free_pgtable_t free_pgtable)
{
#if defined(__x86_64__)
    const uint64_t root_phys = read_cr3();
#else
    const uint64_t root_phys = 0;
    verify_not_reached();
#endif /* defined(__x86_64__) */

    return ptwalker_create_customroot(walker,
                                      root_phys,
                                      virt_addr,
                                      alloc_pgtable,
                                      free_pgtable);
}

void
ptwalker_create(struct pt_walker *const walker,
                const uint64_t root_phys,
                const uint64_t virt_addr,
                const ptwalker_alloc_pgtable_t alloc_pgtable,
                const ptwalker_free_pgtable_t free_pgtable)
{
    return ptwalker_create_customroot(walker,
                                      root_phys,
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
    assert(has_align(virt_addr, PAGE_SIZE));
    assert(has_align(root_phys, PAGE_SIZE));

    walker->level = pgt_get_top_level();
    walker->top_level = walker->level;

    walker->alloc_pgtable = alloc_pgtable;
    walker->free_pgtable = free_pgtable;

    walker->tables[0] = phys_to_virt(root_phys);
    walker->indices[0] = virt_to_pt_index((void *)virt_addr, walker->top_level);

    for (uint8_t level = walker->top_level - 1; level >= 1; level--) {
        const uint8_t index = ptwalker_array_index(walker, level);

        walker->tables[index] = NULL;
        walker->indices[index] = virt_to_pt_index((void *)virt_addr, level);
    }

    for (uint8_t level = walker->top_level; level != PGT_LEVEL_COUNT; level++) {
        walker->tables[level] = NULL;
        walker->indices[level] = UINT16_MAX;
    }
}

pte_t *
ptwalker_table_for_level(const struct pt_walker *const walker,
                         const uint8_t level)
{
    return walker->tables[ptwalker_array_index(walker, level)];
}

uint16_t
ptwalker_table_index(const struct pt_walker *const walker, const uint8_t level)
{
    return walker->indices[ptwalker_array_index(walker, level)];
}

void
ptwalker_set_table_for_level(struct pt_walker *const walker,
                             const uint8_t level,
                             pte_t *const table)
{
    walker->tables[ptwalker_array_index(walker, level)] = table;
}

void
ptwalker_set_index_for_level(struct pt_walker *walker,
                             const uint8_t level,
                             const uint16_t index)
{
    walker->indices[ptwalker_array_index(walker, level)] = index;
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

uint16_t
ptwalker_array_index(const struct pt_walker *const walker, const uint8_t level)
{
    return walker->top_level - level;
}

enum pt_walker_result ptwalker_next(struct pt_walker *const walker) {
    return ptwalker_next_custom(walker,
                                /*level=*/1,
                                /*alloc_parents=*/false,
                                /*alloc_level=*/false,
                                /*should_ref=*/false,
                                /*alloc_pgtable_cb_info=*/NULL,
                                /*free_pgtable_cb_info=*/NULL);
}

static void
reset_levels_lower_than(struct pt_walker *const walker, const uint8_t level) {
    uint16_t index = ptwalker_array_index(walker, level - 1);
    for (int16_t i = level - 1; i >= 1; i--, index++) {
        walker->tables[index] = NULL;
        walker->indices[index] = 0;
    }
}

static void
setup_levels_lower_than(struct pt_walker *const walker,
                        const uint8_t level,
                        pte_t *const first_pte)
{
    pte_t *pte = first_pte;

    uint16_t index = ptwalker_array_index(walker, level - 1);
    int16_t i = level - 1;

    for (; i >= 1; i--, index++) {
        pte_t *const table = phys_to_virt(*pte & PG_PHYS_MASK);

        walker->tables[index] = table;
        walker->indices[index] = 0;

        pte = table;
        if (!pte_is_present(*pte)) {
            walker->level = i;
            return;
        }
    }

    walker->level = i;
}

static inline bool
alloc_single_pte(struct pt_walker *const walker,
                 void *const alloc_pgtable_cb_info,
                 void *const free_pgtable_cb_info,
                 const uint8_t level,
                 const uint16_t index,
                 pte_t *const pte)
{
    struct page *const pt =
        walker->alloc_pgtable(walker, alloc_pgtable_cb_info);

    if (pt == NULL) {
        walker->level = level + 1;
        ptwalker_drop_lowest(walker, free_pgtable_cb_info);

        return false;
    }

    walker->tables[index] = page_to_virt(pt);
    *pte = page_to_phys(pt) | PGT_FLAGS;

    return true;
}

static enum pt_walker_result
alloc_levels_down_to(struct pt_walker *const walker,
                     uint8_t parent_level,
                     uint8_t last_level,
                     const bool alloc_level,
                     const bool should_ref,
                     void *const alloc_pgtable_cb_info,
                     void *const free_pgtable_cb_info)
{
    const ptwalker_alloc_pgtable_t alloc_pgtable = walker->alloc_pgtable;
    assert(alloc_pgtable != NULL);

    pte_t *table = ptwalker_table_for_level(walker, parent_level);
    pte_t *pte = ptwalker_pte_in_level(walker, parent_level);

    uint8_t level = parent_level - 1;
    uint16_t array_index = ptwalker_array_index(walker, level);

    do {
        if (!alloc_single_pte(walker,
                              alloc_pgtable_cb_info,
                              free_pgtable_cb_info,
                              level,
                              array_index,
                              pte))
        {
            walker->level = level + 1;
            ptwalker_drop_lowest(walker, free_pgtable_cb_info);

            return E_PT_WALKER_ALLOC_FAIL;
        }

        if (should_ref) {
            ref_up(&virt_to_page(table)->pte.refcount);
        }

        level--;
        if (level == last_level) {
            if (!alloc_level) {
                break;
            }
        } else if (level < last_level) {
            break;
        }

        pte = ptwalker_pte_in_level(walker, level + 1);
        table = ptwalker_pte_in_level(walker, level + 1);

        array_index++;
    } while (true);

    walker->level = level + 1;
    return E_PT_WALKER_OK;
}

enum pt_walker_result
ptwalker_next_custom(struct pt_walker *walker,
                     uint8_t level,
                     const bool alloc_parents,
                     const bool alloc_level,
                     const bool should_ref,
                     void *const alloc_pgtable_cb_info,
                     void *const free_pgtable_cb_info)
{
    const uint8_t orig_level = level;
    uint16_t index_into_array = ptwalker_array_index(walker, level);

    for (; level <= walker->top_level; level++, index_into_array--) {
        pte_t *const table = walker->tables[index_into_array];
        if (table == NULL) {
            continue;
        }

        walker->indices[index_into_array]++;
        if (walker->indices[index_into_array] == PGT_COUNT) {
            if (level == walker->top_level) {
                // Reset all of walker, except the root pointer
                walker->indices[index_into_array] = 0;
                walker->level = PTWALKER_DONE;

                reset_levels_lower_than(walker, level);
                return E_PT_WALKER_OK;
            }

            continue;
        }

        if (level == orig_level) {
            if (!alloc_level) {
                return E_PT_WALKER_OK;
            }

            pte_t *const pte = ptwalker_pte_in_level(walker, level + 1);
            if (pte_is_present(*pte)) {
                return E_PT_WALKER_OK;
            }

            assert(walker->alloc_pgtable != NULL);
            if (!alloc_single_pte(walker,
                                  alloc_pgtable_cb_info,
                                  free_pgtable_cb_info,
                                  level,
                                  index_into_array,
                                  pte))
            {
                walker->level = level + 1;
                ptwalker_drop_lowest(walker, free_pgtable_cb_info);

                return E_PT_WALKER_ALLOC_FAIL;
            }

            return E_PT_WALKER_OK;
        }

        pte_t *const pte = &table[walker->indices[index_into_array]];
        const pte_t entry = *pte;

        if (!pte_is_present(entry)) {
            break;
        }

        if (pte_is_large(entry, level)) {
            break;
        }

        setup_levels_lower_than(walker, level, pte);
        walker->level = level;

        return E_PT_WALKER_OK;
    }

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
    pte_t *const pte = ptwalker_pte_in_level(walker, level + 1);

    *pte = page_to_phys(page) | PGT_FLAGS;

    ref_up(&phys_to_page(*pte & PG_PHYS_MASK)->pte.refcount);
    ref_up(&page->pte.refcount);
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

void ptwalker_drop_lowest(struct pt_walker *const walker, void *const cb_info) {
    const ptwalker_free_pgtable_t free_pgtable = walker->free_pgtable;
    assert(free_pgtable != NULL);

    uint8_t level = walker->level;
    uint16_t index = ptwalker_array_index(walker, level);

    walker->tables[index] = NULL;
    walker->indices[index] = 0;

    for (level++, index--; level <= walker->top_level; level++, index--) {
        pte_t *const table = walker->tables[index];
        struct page *const pt = virt_to_page(table);

        if (!ref_down(&pt->pte.refcount)) {
            break;
        }

        free_pgtable(walker, pt, cb_info);
    }

    walker->level = level;
}