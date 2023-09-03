/*
 * kernel/mm/pgmap.c
 * © suhas pai
 */

#include "dev/printk.h"
#include "lib/align.h"

#include "mm/early.h"
#include "mm/page_alloc.h"
#include "mm/pageop.h"

#include "pgmap.h"

static struct page *
ptwalker_early_alloc_pgtable_cb(struct pt_walker *const walker,
                                void *const cb_info)
{
    (void)walker;
    (void)cb_info;

    // We don't have a structpage-table setup yet when this function is called,
    // but because ptwalker never dereferences the page, we can return a pointer
    // to where the page would've been.

    const uint64_t phys = early_alloc_page();
    if (phys != INVALID_PHYS) {
        return phys_to_page(phys);
    }

    panic("mm: failed to setup page-structs, ran out of memory\n");
}

enum map_result {
    MAP_DONE,
    MAP_CONTINUE,
    MAP_RESTART
};

struct current_split_info {
    struct range phys_range;
    uint64_t virt_addr;

    struct pt_walker deref;
};

static void
split_large_page(struct pt_walker *const walker,
                 struct current_split_info *const curr_split,
                 struct pageop *const pageop,
                 const pgt_level_t level,
                 const struct pgmap_options *const options)
{
    pte_t *const table = walker->tables[walker->level - 1];
    pte_t *const pte = table + walker->indices[walker->level - 1];

    curr_split->virt_addr = ptwalker_get_virt_addr(walker);
    curr_split->phys_range =
        range_create(pte_to_phys(pte_read(pte)),
                     PAGE_SIZE_AT_LEVEL(walker->level));

    if (!options->is_in_early) {
        pageop_flush_pte(pageop, pte, walker->level);
    } else {
        pte_write(pte, /*value=*/0);
    }

    for (pgt_level_t i = 1; i <= level; i++) {
        walker->tables[i - 1] = NULL;
        walker->indices[i - 1] = 0;
    }
}

static bool
pgmap_with_ptwalker(struct pt_walker *const walker,
                    struct current_split_info *const curr_split,
                    struct pageop *const pageop,
                    const struct range phys_range,
                    const uint64_t virt_addr,
                    const struct pgmap_options *options);

static void
finish_split_info(struct pt_walker *const walker,
                  struct current_split_info *const curr_split,
                  struct pageop *const pageop,
                  const uint64_t virt,
                  const struct pgmap_options *const options)
{
    if (curr_split->deref.level != 0) {
        ptwalker_deref_from_level(&curr_split->deref,
                                  curr_split->deref.level,
                                  /*free_pgtable_cb_info=*/NULL);
    }

    const uint64_t virt_end =
        check_add_assert(curr_split->virt_addr, curr_split->phys_range.size);

    const uint64_t walker_virt_addr = ptwalker_get_virt_addr(walker);
    if (walker_virt_addr >= virt_end) {
        pageop_finish(pageop);
        return;
    }

    const uint64_t offset = virt_end - walker_virt_addr;
    const struct range phys_range =
        range_from_index(curr_split->phys_range, offset);

    struct current_split_info new_curr_split = {};
    pgmap_with_ptwalker(walker,
                        &new_curr_split,
                        pageop,
                        phys_range,
                        virt,
                        options);
}

static void
free_tables_below(struct pt_walker *const walker, const pgt_level_t level_below)
{
    const pgt_level_t level = level_below - 1;
    if (level > 1) {
        if (pte_level_can_have_large(level)) {
            pte_t *pte = walker->tables[level - 1];
            const pte_t *const end = pte + PGT_COUNT;

            for (; pte != end; pte++) {
                if (!pte_is_large(pte_read(pte))) {
                    free_tables_below(walker, level);
                }
            }
        } else {
            for (pgt_index_t i = 0; i != PGT_COUNT; i++) {
                free_tables_below(walker, level);
            }
        }
    }

    free_page(virt_to_page(walker->tables[level - 1]));
    for (pgt_level_t i = 1; i <= level; i++) {
        walker->tables[i - 1] = NULL;
        walker->indices[i - 1] = 0;
    }
}

enum override_result {
    OVERRIDE_OK,
    OVERRIDE_DONE
};

static enum override_result
override_pte(struct pt_walker *const walker,
             struct current_split_info *const curr_split,
             struct pageop *const pageop,
             const uint64_t phys_begin,
             const uint64_t virt_begin,
             uint64_t *const offset_in,
             const uint64_t size,
             const pgt_level_t level,
             const pte_t new_pte_value,
             const struct pgmap_options *const options)
{
    if (level > walker->level) {
        free_tables_below(walker, level);
        pte_t *const pte =
            &walker->tables[level - 1][walker->indices[level - 1]];

        pte_write(pte, new_pte_value);
        return OVERRIDE_OK;
    }

    pte_t entry = 0;
    if (level < walker->level) {
        if (!ptwalker_points_to_largepage(walker)) {
            const enum pt_walker_result ptwalker_result =
                ptwalker_fill_in_to(walker,
                                    level,
                                    /*should_ref=*/!options->is_in_early,
                                    options->alloc_pgtable_cb_info,
                                    options->free_pgtable_cb_info);

            if (ptwalker_result != E_PT_WALKER_OK) {
                panic("mm: failed to pgmap, result=%d\n", ptwalker_result);
            }

            pte_t *const pte =
                &walker->tables[level - 1][walker->indices[level - 1]];

            pte_write(pte, new_pte_value);
            return OVERRIDE_OK;
        }

        const pgt_index_t index = walker->indices[walker->level - 1];
        entry = pte_read(&walker->tables[walker->level - 1][index]);
    } else {
        pte_t *const pte =
            &walker->tables[level - 1][walker->indices[level - 1]];

        entry = pte_read(pte);
        if (!pte_is_present(entry)) {
            pte_write(pte, new_pte_value);
            return OVERRIDE_OK;
        }
    }

    // Avoid overwriting if we have a page of the same size, or greater size,
    // that is mapped with the same flags.

    uint64_t phys_addr = phys_begin + *offset_in;
    if (pte_to_phys(entry) == phys_addr &&
        pte_flags_equal(entry, walker->level, options->pte_flags))
    {
        *offset_in += PAGE_SIZE_AT_LEVEL(walker->level);
        if (*offset_in >= size) {
            return OVERRIDE_DONE;
        }

        const enum pt_walker_result result =
            ptwalker_next_with_options(walker,
                                       walker->level,
                                       /*alloc_parents=*/false,
                                       /*alloc_level=*/false,
                                       /*should_ref=*/!options->is_in_early,
                                       /*alloc_pgtable_cb_info=*/NULL,
                                       /*free_pgtable_cb_info=*/NULL);

        if (result != E_PT_WALKER_OK) {
            panic("mm: failed to pgmap, result=%d", result);
        }

        *offset_in = phys_addr - phys_begin;
        return OVERRIDE_OK;
    }

    if (level < walker->level) {
        split_large_page(walker, curr_split, pageop, level, options);
    } else {
        pageop_flush_address(pageop, virt_begin + *offset_in);
    }

    pte_t *const pte =
        &walker->tables[level - 1][walker->indices[level - 1]];

    pte_write(pte, new_pte_value);
    return OVERRIDE_OK;
}

enum map_result
map_normal(struct pt_walker *const walker,
           struct current_split_info *const curr_split,
           struct pageop *const pageop,
           const uint64_t phys_begin,
           const uint64_t virt_begin,
           uint64_t *const offset_in,
           const uint64_t size,
           const struct pgmap_options *const options)
{
    uint64_t offset = *offset_in;
    if (offset >= size) {
        return MAP_DONE;
    }

    const bool should_ref = !options->is_in_early;
    const uint64_t pte_flags = options->pte_flags;

    void *const alloc_pgtable_cb_info = options->alloc_pgtable_cb_info;
    void *const free_pgtable_cb_info = options->free_pgtable_cb_info;

    enum pt_walker_result ptwalker_result = E_PT_WALKER_OK;
    if (options->is_overwrite) {
        do {
            ptwalker_result =
                ptwalker_fill_in_to(walker,
                                    /*level=*/1,
                                    should_ref,
                                    options->alloc_pgtable_cb_info,
                                    options->free_pgtable_cb_info);

            if (ptwalker_result != E_PT_WALKER_OK) {
            panic:
                panic("mm: failed to pgmap, result=%d\n", ptwalker_result);
            }

            do {
                const pte_t new_pte_value =
                    phys_create_pte(phys_begin + offset) |
                    PTE_LEAF_FLAGS |
                    pte_flags;
                const enum override_result override_result =
                    override_pte(walker,
                                 curr_split,
                                 pageop,
                                 phys_begin,
                                 virt_begin,
                                 &offset,
                                 size,
                                 /*level=*/1,
                                 new_pte_value,
                                 options);

                switch (override_result) {
                    case OVERRIDE_OK:
                        break;
                    case OVERRIDE_DONE:
                        return MAP_DONE;
                }

                ptwalker_result =
                    ptwalker_next_with_options(walker,
                                               /*level=*/1,
                                               /*alloc_parents=*/false,
                                               /*alloc_level=*/false,
                                               should_ref,
                                               alloc_pgtable_cb_info,
                                               free_pgtable_cb_info);

                if (ptwalker_result != E_PT_WALKER_OK) {
                    goto panic;
                }

                offset += PAGE_SIZE;
                if (offset == size) {
                    *offset_in = offset;
                    return MAP_DONE;
                }

                if (walker->indices[0] == 0) {
                    if (walker->indices[1] == 0) {
                        *offset_in = offset;
                        return MAP_RESTART;
                    }

                    break;
                }
            } while (true);
        } while (true);
    } else {
        uint64_t phys_addr = phys_begin + offset;
        const uint64_t phys_end = phys_begin + size;

        do {
            ptwalker_result =
                ptwalker_fill_in_to(walker,
                                    /*level=*/1,
                                    should_ref,
                                    options->alloc_pgtable_cb_info,
                                    options->free_pgtable_cb_info);

            if (ptwalker_result != E_PT_WALKER_OK) {
                panic("mm: failed to pgmap, result=%d\n", ptwalker_result);
            }

            pte_t *const table = walker->tables[0];
            pte_t *pte = table + walker->indices[0];
            const pte_t *const end = table + PGT_COUNT;

            do {
                const pte_t new_pte_value =
                    phys_create_pte(phys_addr) | PTE_LEAF_FLAGS | pte_flags;

                pte_write(pte, new_pte_value);
                phys_addr += PAGE_SIZE;

                pte++;
                if (pte == end) {
                    walker->indices[0] = PGT_COUNT - 1;
                    ptwalker_result =
                        ptwalker_next_with_options(walker,
                                                   /*level=*/1,
                                                   /*alloc_parents=*/false,
                                                   /*alloc_level=*/false,
                                                   should_ref,
                                                   alloc_pgtable_cb_info,
                                                   free_pgtable_cb_info);

                    if (ptwalker_result != E_PT_WALKER_OK) {
                        goto panic;
                    }

                    if (walker->indices[1] == 0) {
                        *offset_in = phys_addr - phys_begin;
                        return MAP_RESTART;
                    }

                    break;
                }

                if (phys_addr == phys_end) {
                    walker->indices[0] = (pte - table);
                    *offset_in = phys_addr - phys_begin;

                    return MAP_DONE;
                }
            } while (true);
        } while (true);
    }
}

enum map_result
map_large_at_top_level_overwrite(struct pt_walker *const walker,
                                 struct current_split_info *const curr_split,
                                 struct pageop *const pageop,
                                 const uint64_t phys_begin,
                                 const uint64_t virt_begin,
                                 uint64_t *const offset_in,
                                 const uint64_t size,
                                 const struct pgmap_options *const options)
{
    const pgt_level_t level = walker->top_level;

    const uint64_t largepage_size = PAGE_SIZE_AT_LEVEL(level);
    const uint64_t pte_flags = options->pte_flags;

    void *const alloc_pgtable_cb_info = options->alloc_pgtable_cb_info;
    void *const free_pgtable_cb_info = options->free_pgtable_cb_info;

    const bool should_ref = !options->is_in_early;
    uint64_t offset = *offset_in;

    do {
        const pte_t new_pte_value =
            phys_create_pte(phys_begin + offset) |
            PTE_LARGE_FLAGS(level) |
            pte_flags;

        const enum override_result override_result =
            override_pte(walker,
                            curr_split,
                            pageop,
                            phys_begin,
                            virt_begin,
                            offset_in,
                            size,
                            level,
                            new_pte_value,
                            options);

        switch (override_result) {
            case OVERRIDE_OK:
                break;
            case OVERRIDE_DONE:
                return MAP_DONE;
        }

        const enum pt_walker_result ptwalker_result =
            ptwalker_next_with_options(walker,
                                       level,
                                       /*alloc_parents=*/false,
                                       /*alloc_level=*/false,
                                       should_ref,
                                       alloc_pgtable_cb_info,
                                       free_pgtable_cb_info);

        if (ptwalker_result != E_PT_WALKER_OK) {
            panic("mm: failed to pgmap, result=%d\n", ptwalker_result);
        }

        offset += largepage_size;
        if (offset == size) {
            *offset_in = offset;
            return MAP_DONE;
        }
    } while (offset + largepage_size <= size);

    *offset_in = offset;
    return MAP_CONTINUE;
}

enum map_result
map_large_at_top_level_no_overwrite(struct pt_walker *const walker,
                                    const uint64_t phys_begin,
                                    uint64_t *const offset_in,
                                    const uint64_t size,
                                    const struct pgmap_options *const options)
{
    const pgt_level_t level = walker->top_level;

    const uint64_t largepage_size = PAGE_SIZE_AT_LEVEL(level);
    const uint64_t pte_flags = options->pte_flags;

    uint64_t offset = *offset_in;

    uint64_t phys_addr = phys_begin + offset;
    const uint64_t phys_end = phys_begin + size;

    pte_t *const table = walker->tables[level - 1];
    pte_t *pte = table + walker->indices[level - 1];

    do {
        const pte_t new_pte_value =
            phys_create_pte(phys_addr) | PTE_LARGE_FLAGS(level) | pte_flags;

        pte_write(pte, new_pte_value);
        phys_addr += largepage_size;

        if (phys_addr == phys_end) {
            *offset_in = phys_addr - phys_begin;
            return MAP_CONTINUE;
        }

        pte++;
    } while (phys_addr + largepage_size <= phys_end);

    *offset_in = phys_addr - phys_begin;
    return MAP_CONTINUE;
}

enum map_result
map_large_at_level_overwrite(struct pt_walker *const walker,
                             struct current_split_info *const curr_split,
                             struct pageop *const pageop,
                             const uint64_t phys_begin,
                             const uint64_t virt_begin,
                             uint64_t *const offset_in,
                             const uint64_t size,
                             const pgt_level_t level,
                             const struct pgmap_options *const options)
{
    const uint64_t largepage_size = PAGE_SIZE_AT_LEVEL(level);
    const uint64_t pte_flags = options->pte_flags;

    void *const alloc_pgtable_cb_info = options->alloc_pgtable_cb_info;
    void *const free_pgtable_cb_info = options->free_pgtable_cb_info;

    const bool should_ref = !options->is_in_early;
    uint64_t offset = *offset_in;

    enum pt_walker_result ptwalker_result =
        ptwalker_fill_in_to(walker,
                            level,
                            should_ref,
                            alloc_pgtable_cb_info,
                            free_pgtable_cb_info);

    if (ptwalker_result != E_PT_WALKER_OK) {
    panic:
        panic("mm: failed to pgmap, result=%d\n", ptwalker_result);
    }

    do {
        const pte_t new_pte_value =
            phys_create_pte(phys_begin + offset) |
            PTE_LARGE_FLAGS(level) |
            pte_flags;
        const enum override_result override_result =
            override_pte(walker,
                            curr_split,
                            pageop,
                            phys_begin,
                            virt_begin,
                            offset_in,
                            size,
                            level,
                            new_pte_value,
                            options);

        switch (override_result) {
            case OVERRIDE_OK:
                break;
            case OVERRIDE_DONE:
                return MAP_DONE;
        }

        ptwalker_result =
            ptwalker_next_with_options(walker,
                                       level,
                                       /*alloc_parents=*/false,
                                       /*alloc_level=*/false,
                                       should_ref,
                                       alloc_pgtable_cb_info,
                                       free_pgtable_cb_info);

        if (ptwalker_result != E_PT_WALKER_OK) {
            goto panic;
        }

        offset += largepage_size;
        if (offset == size) {
            *offset_in = offset;
            return MAP_DONE;
        }

        if (walker->indices[walker->level - 1] == 0) {
            if (walker->indices[walker->level] == 0) {
                *offset_in = offset;
                return MAP_RESTART;
            }

            break;
        }
    } while (offset + largepage_size <= size);

    *offset_in = offset;
    return MAP_CONTINUE;
}

enum map_result
map_large_at_level_no_overwrite(struct pt_walker *const walker,
                                const uint64_t phys_begin,
                                uint64_t *const offset_in,
                                const uint64_t size,
                                const pgt_level_t level,
                                const struct pgmap_options *const options)
{
    const uint64_t largepage_size = PAGE_SIZE_AT_LEVEL(level);
    const uint64_t pte_flags = options->pte_flags;

    void *const alloc_pgtable_cb_info = options->alloc_pgtable_cb_info;
    void *const free_pgtable_cb_info = options->free_pgtable_cb_info;

    const bool should_ref = !options->is_in_early;
    uint64_t offset = *offset_in;

    uint64_t phys_addr = phys_begin + offset;
    const uint64_t phys_end = phys_begin + size;

    do {
        enum pt_walker_result ptwalker_result =
            ptwalker_fill_in_to(walker,
                                level,
                                should_ref,
                                alloc_pgtable_cb_info,
                                free_pgtable_cb_info);

        if (ptwalker_result != E_PT_WALKER_OK) {
        panic:
            panic("mm: failed to pgmap, result=%d\n", ptwalker_result);
        }

        pte_t *const table = walker->tables[level - 1];
        pte_t *pte = table + walker->indices[level - 1];
        const pte_t *const end = table + PGT_COUNT;

        do {
            const pte_t new_pte_value =
                phys_create_pte(phys_addr) | PTE_LARGE_FLAGS(level) | pte_flags;

            pte_write(pte, new_pte_value);
            phys_addr += largepage_size;

            if (phys_addr == phys_end) {
                walker->indices[level - 1] = (pte - table);
                *offset_in = phys_addr - phys_begin;

                return MAP_DONE;
            }

            pte++;
            if (pte == end) {
                walker->indices[level - 1] = PGT_COUNT - 1;
                ptwalker_result =
                    ptwalker_next_with_options(walker,
                                               level,
                                               /*alloc_parents=*/false,
                                               /*alloc_level=*/false,
                                               should_ref,
                                               alloc_pgtable_cb_info,
                                               free_pgtable_cb_info);

                if (ptwalker_result != E_PT_WALKER_OK) {
                    goto panic;
                }

                if (walker->indices[walker->level] == 0) {
                    *offset_in = phys_addr - phys_begin;
                    return MAP_RESTART;
                }

                break;
            }

            if (phys_addr + largepage_size > phys_end) {
                walker->indices[level - 1] = (pte - table);
                *offset_in = phys_addr - phys_begin;

                return MAP_CONTINUE;
            }
        } while (true);
    } while (true);
}

enum map_result
map_large_at_level(struct pt_walker *const walker,
                   struct current_split_info *const curr_split,
                   struct pageop *const pageop,
                   const uint64_t phys_begin,
                   const uint64_t virt_begin,
                   uint64_t *const offset_in,
                   const uint64_t size,
                   const pgt_level_t level,
                   const struct pgmap_options *const options)
{
    if (walker->indices[level - 2] != 0) { // Get idx of level - 1 == level - 2
        return MAP_CONTINUE;
    }

    const uint64_t largepage_size = PAGE_SIZE_AT_LEVEL(level);
    uint64_t offset = *offset_in;

    if (!has_align(phys_begin + offset, largepage_size) ||
        offset + largepage_size > size)
    {
        return MAP_CONTINUE;
    }

    if (level == pgt_get_top_level()) {
        if (options->is_overwrite) {
            return map_large_at_top_level_overwrite(walker,
                                                    curr_split,
                                                    pageop,
                                                    phys_begin,
                                                    virt_begin,
                                                    offset_in,
                                                    size,
                                                    options);
        }

        return map_large_at_top_level_no_overwrite(walker,
                                                   phys_begin,
                                                   offset_in,
                                                   size,
                                                   options);
    }

    if (options->is_overwrite) {
        return map_large_at_level_overwrite(walker,
                                            curr_split,
                                            pageop,
                                            phys_begin,
                                            virt_begin,
                                            offset_in,
                                            size,
                                            level,
                                            options);
    }

    return map_large_at_level_no_overwrite(walker,
                                           phys_begin,
                                           offset_in,
                                           size,
                                           level,
                                           options);
}

static bool
pgmap_with_ptwalker(struct pt_walker *const walker,
                    struct current_split_info *const curr_split,
                    struct pageop *const pageop,
                    const struct range phys_range,
                    const uint64_t virt_begin,
                    const struct pgmap_options *const options)
{
    if (range_empty(phys_range)) {
        printk(LOGLEVEL_WARN, "pgmap_at(): phys-range is empty\n");
        return false;
    }

    uint64_t phys_end = 0;
    if (!range_get_end(phys_range, &phys_end)) {
        printk(LOGLEVEL_WARN,
               "pgmap_at(): phys-range goes beyond end of address-space\n");
        return false;
    }

    uint64_t phys_addr = phys_range.front;
    if (!range_has_align(phys_range, PAGE_SIZE)) {
        printk(LOGLEVEL_WARN,
               "pgmap_at(): phys-range isn't aligned to PAGE_SIZE\n");
        return false;
    }

    uint64_t offset = 0;
    const uint64_t supports_largepage_at_level_mask =
        options->supports_largepage_at_level_mask;

    do {
    start:
        for (int16_t index = countof(LARGEPAGE_LEVELS) - 1; index >= 0; index--)
        {
            const pgt_level_t level = LARGEPAGE_LEVELS[index];
            if (!(supports_largepage_at_level_mask & (1ull << level))) {
                continue;
            }

            const enum map_result result =
                map_large_at_level(walker,
                                   curr_split,
                                   pageop,
                                   phys_range.front,
                                   virt_begin,
                                   &offset,
                                   phys_range.size,
                                   level,
                                   options);

            switch (result) {
                case MAP_DONE:
                    finish_split_info(walker,
                                      curr_split,
                                      pageop,
                                      virt_begin + offset,
                                      options);
                    return true;
                case MAP_CONTINUE:
                    break;
                case MAP_RESTART:
                    goto start;
            }
        }

        const enum map_result result =
            map_normal(walker,
                       curr_split,
                       pageop,
                       phys_range.front,
                       virt_begin,
                       &offset,
                       phys_range.size,
                       options);

        switch (result) {
            case MAP_DONE:
                finish_split_info(walker,
                                  curr_split,
                                  pageop,
                                  virt_begin + offset,
                                  options);
                return true;
            case MAP_CONTINUE:
            case MAP_RESTART:
                break;
        }
    } while (phys_addr < phys_end);

    finish_split_info(walker, curr_split, pageop, virt_begin + offset, options);
    return true;
}

bool
pgmap_at(const struct pagemap *const pagemap,
         struct range phys_range,
         uint64_t virt_addr,
         const struct pgmap_options *const options)
{
    struct pt_walker walker;
    if (options->is_in_early) {
        ptwalker_create_for_pagemap(&walker,
                                    pagemap,
                                    virt_addr,
                                    ptwalker_early_alloc_pgtable_cb,
                                    /*free_pgtable=*/NULL);
    } else {
        ptwalker_default_for_pagemap(&walker, pagemap, virt_addr);
    }

    struct current_split_info curr_split = {
        .phys_range = range_create_empty(),
        .virt_addr = 0,
        .deref = PT_WALKER_INIT()
    };

    struct pageop pageop;
    pageop_init(&pageop);

    if (ptwalker_points_to_largepage(&walker)) {
        const uint64_t walker_virt_addr = ptwalker_get_virt_addr(&walker);
        if (walker_virt_addr < virt_addr) {
            uint64_t offset = virt_addr - walker_virt_addr;
            const uint64_t phys_addr = phys_range.front - offset;

            pte_t *const pte =
                walker.tables[walker.level - 1] +
                walker.indices[walker.level - 1];

            const pte_t entry = pte_read(pte);
            if (pte_to_phys(entry) == phys_addr &&
                pte_flags_equal(entry, walker.level, options->pte_flags))
            {
                offset += PAGE_SIZE_AT_LEVEL(walker.level);
                if (!range_has_index(phys_range, offset)) {
                    return OVERRIDE_DONE;
                }

                phys_range = range_from_index(phys_range, offset);
                virt_addr += offset;
            } else {
                split_large_page(&walker,
                                 &curr_split,
                                 &pageop,
                                 walker.level,
                                 options);

                const struct range largepage_phys_range =
                    range_create(curr_split.phys_range.front, offset);
                const bool result =
                    pgmap_with_ptwalker(&walker,
                                        /*curr_split=*/NULL,
                                        &pageop,
                                        largepage_phys_range,
                                        walker_virt_addr,
                                        options);

                if (!result) {
                    return result;
                }

                curr_split.virt_addr += offset;
                curr_split.phys_range =
                    range_from_index(curr_split.phys_range, offset);
            }
        }
    }

    return
        pgmap_with_ptwalker(&walker,
                            &curr_split,
                            &pageop,
                            phys_range,
                            virt_addr,
                            options);
}