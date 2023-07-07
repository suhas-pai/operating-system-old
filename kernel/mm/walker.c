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

void pgtwalker_default(struct pgt_walker *const walker, const uint64_t virt) {
#if defined(__x86_64__)
    const uint64_t root_phys = read_cr3();
#else
    const uint64_t root_phys = 0;
    verify_not_reached();
#endif /* defined(__x86_64__) */

    return pgtwalker_create_customroot(walker, virt, root_phys);
}

void
pgtwalker_create(struct pgt_walker *const walker,
                 struct pagemap *const pagemap,
                 const uint64_t virt_addr)
{
    return pgtwalker_create_customroot(walker,
                                       virt_addr,
                                       page_to_phys(pagemap->root));
}

void
pgtwalker_create_customroot(struct pgt_walker *const walker,
                            const uint64_t virt_addr,
                            const uint64_t root_phys)
{
    assert(has_align(virt_addr, PAGE_SIZE));
    assert(has_align(root_phys, PAGE_SIZE));

    walker->level = pgt_get_top_level();
    walker->top_level = walker->level;

    walker->tables[0] = phys_to_virt(root_phys);
    walker->indices[0] = (int16_t)PG_LEVEL_INDEX(virt_addr, walker->top_level);

    for (uint8_t level = walker->top_level - 1; level >= 1; level--) {
        const uint8_t index = pgtwalker_index_into_array(walker, level);

        walker->tables[index] = NULL;
        walker->indices[index] = (int16_t)PG_LEVEL_INDEX(virt_addr, level);
    }

    for (uint8_t level = walker->top_level; level != PGT_LEVEL_COUNT; level++) {
        walker->indices[level] = -1;
    }
}

pte_t *
pgtwalker_table_for_level(struct pgt_walker *const walker,
                          const uint8_t level)
{
    return walker->tables[pgtwalker_index_into_array(walker, level)];
}

int16_t
pgtwalker_table_index_for_level(struct pgt_walker *const walker,
                                const uint8_t level)
{
    return walker->indices[pgtwalker_index_into_array(walker, level)];
}

pte_t *
pgtwalker_pte_in_level(struct pgt_walker *const walker, const uint8_t level) {
    pte_t *const table = pgtwalker_table_for_level(walker, level);
    if (table != NULL) {
        return table + pgtwalker_table_index_for_level(walker, level);
    }

    return NULL;
}

uint16_t
pgtwalker_index_into_array(struct pgt_walker *const walker, const uint8_t level)
{
    return walker->top_level - level;
}

enum pgt_walker_result pgtwalker_next(struct pgt_walker *const walker) {
    return pgtwalker_next_custom(walker,
                                 /*level=*/1,
                                 /*alloc_if_not_present=*/true,
                                 /*alloc_parents_if_not_present=*/true);
}

static void
null_levels_lower_than(struct pgt_walker *const walker, const uint8_t level) {
    for (uint64_t i = level - 1; i >= 1; i--) {
        const uint64_t index = pgtwalker_index_into_array(walker, i);

        walker->tables[index] = NULL;
        walker->indices[index] = 0;
    }
}

static enum pgt_walker_result
alloc_levels(struct pgt_walker *const walker,
             const uint8_t level,
             const uint8_t orig_level,
             const bool alloc_parents_if_not_present)
{
    if (level != orig_level) {
        if (!alloc_parents_if_not_present) {
            null_levels_lower_than(walker, level);
            walker->level = level;

            return E_PGT_WALKER_OK;
        }

        // TODO: Alloc
        return E_PGT_WALKER_OK;
    } else {
        // TODO: Alloc
        return E_PGT_WALKER_OK;
    }
}

enum pgt_walker_result
pgtwalker_next_custom(struct pgt_walker *const walker,
                      uint8_t level,
                      const bool alloc_if_not_present,
                      const bool alloc_parents_if_not_present)
{
    const uint8_t orig_level = level;
    for (; level <= walker->top_level; level++) {
        const uint16_t index_into_array =
            pgtwalker_index_into_array(walker, level);

        pte_t *const table = walker->tables[index_into_array];
        if (table == NULL) {
            continue;
        }

        walker->indices[index_into_array]++;
        if (walker->indices[index_into_array] == PGT_COUNT) {
            if (level == walker->top_level) {
                walker->level = PGWALKER_DONE;
                break;
            }

            continue;
        }

        if (level == orig_level) {
            if (!alloc_if_not_present) {
                null_levels_lower_than(walker, level);
                return E_PGT_WALKER_OK;
            }
        }

        const pte_t entry = table[walker->indices[index_into_array]];
        if (!pte_is_present(entry)) {
            break;
        }

        if (pte_is_large(entry, level)) {
            walker->level = level;
            null_levels_lower_than(walker, level);

            return E_PGT_WALKER_OK;
        }

        verify_not_reached();
    }

    return alloc_levels(walker,
                        level,
                        orig_level,
                        alloc_parents_if_not_present);
}