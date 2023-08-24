/*
 * kernel/mm/vmap.c
 * © suhas pai
 */

#include "dev/printk.h"
#include "lib/align.h"

#include "vmap.h"

enum map_result {
    MAP_DONE,
    MAP_CONTINUE,
    MAP_RESTART
};

enum map_result
map_normal(struct pt_walker *const walker,
           uint64_t *const phys_addr_in,
           const uint64_t phys_end,
           const uint64_t flags,
           const bool should_ref,
           const bool is_overwrite,
           void *const alloc_pgtable_cb_info,
           void *const free_pgtable_cb_info)
{
    uint64_t phys_addr = *phys_addr_in;
    if (phys_addr + PAGE_SIZE > phys_end) {
        return MAP_DONE;
    }

    struct pageop pageop = {0};
    pageop_init(&pageop);

    enum pt_walker_result ptwalker_result =
        ptwalker_fill_in_to(walker,
                            /*level=*/1,
                            should_ref,
                            alloc_pgtable_cb_info,
                            free_pgtable_cb_info);

    if (is_overwrite) {
        do {
            if (ptwalker_result != E_PT_WALKER_OK) {
                panic("mm: failed to setup kernel pagemap, result=%d\n",
                      ptwalker_result);
            }

            pte_t *const pte = &walker->tables[0][walker->indices[0]];
            if (pte_is_present(*pte)) {
                // TODO:
            }

            *pte = phys_create_pte(phys_addr) | PTE_LEAF_FLAGS | flags;
            ptwalker_result =
                ptwalker_next_with_options(walker,
                                           /*level=*/1,
                                           /*alloc_parents=*/false,
                                           /*alloc_level=*/false,
                                           /*should_ref=*/false,
                                           alloc_pgtable_cb_info,
                                           free_pgtable_cb_info);

            phys_addr += PAGE_SIZE;
            if (phys_addr == phys_end) {
                *phys_addr_in = phys_addr;
                return MAP_DONE;
            }

            if (walker->indices[0] == 0) {
                *phys_addr_in = phys_addr;
                return MAP_RESTART;
            }
        } while (phys_addr + PAGE_SIZE <= phys_end);
    } else {
        do {
            if (ptwalker_result != E_PT_WALKER_OK) {
                panic("mm: failed to setup kernel pagemap, result=%d\n",
                      ptwalker_result);
            }

            walker->tables[0][walker->indices[0]] =
                phys_create_pte(phys_addr) | PTE_LEAF_FLAGS | flags;

            ptwalker_result =
                ptwalker_next_with_options(walker,
                                           /*level=*/1,
                                           /*alloc_parents=*/false,
                                           /*alloc_level=*/false,
                                           /*should_ref=*/false,
                                           alloc_pgtable_cb_info,
                                           free_pgtable_cb_info);

            phys_addr += PAGE_SIZE;
            if (phys_addr == phys_end) {
                *phys_addr_in = phys_addr;
                return MAP_DONE;
            }

            if (walker->indices[0] == 0) {
                *phys_addr_in = phys_addr;
                return MAP_RESTART;
            }
        } while (phys_addr + PAGE_SIZE <= phys_end);
    }

    *phys_addr_in = phys_addr;
    return MAP_DONE;
}

enum map_result
map_large_at_level(struct pt_walker *const walker,
                   uint64_t *const phys_addr_in,
                   const uint64_t phys_end,
                   const pgt_level_t level,
                   const uint64_t flags,
                   const bool should_ref,
                   const bool is_overwrite,
                   void *const alloc_pgtable_cb_info,
                   void *const free_pgtable_cb_info)
{
    const uint64_t largepage_size = PAGE_SIZE_AT_LEVEL(level);
    uint64_t phys_addr = *phys_addr_in;

    if (!has_align(phys_addr, largepage_size) ||
        walker->indices[level - 2] != 0 || // Get idx of level - 1 == level - 2
        phys_addr + largepage_size > phys_end)
    {
        return MAP_CONTINUE;
    }

    const pgt_level_t top_level = pgt_get_top_level();
    enum pt_walker_result ptwalker_result =
        ptwalker_fill_in_to(walker,
                            level,
                            should_ref,
                            alloc_pgtable_cb_info,
                            free_pgtable_cb_info);

    if (level == top_level) {
        if (is_overwrite) {
            do {
                if (ptwalker_result != E_PT_WALKER_OK) {
                    panic("mm: failed to setup kernel pagemap, result=%d\n",
                          ptwalker_result);
                }

                pte_t *const pte =
                    &walker->tables[level - 1][walker->indices[level - 1]];

                if (pte_is_present(*pte)) {
                    // TODO:
                }

                *pte =
                    phys_create_pte(phys_addr) | PTE_LARGE_FLAGS(level) | flags;
                ptwalker_result =
                    ptwalker_next_with_options(walker,
                                               level,
                                               /*alloc_parents=*/false,
                                               /*alloc_level=*/false,
                                               /*should_ref=*/false,
                                               alloc_pgtable_cb_info,
                                               free_pgtable_cb_info);

                phys_addr += largepage_size;
                if (phys_addr == phys_end) {
                    *phys_addr_in = phys_addr;
                    return MAP_DONE;
                }
            } while (phys_addr + largepage_size <= phys_end);
        } else {
            do {
                if (ptwalker_result != E_PT_WALKER_OK) {
                    panic("mm: failed to setup kernel pagemap, result=%d\n",
                          ptwalker_result);
                }

                walker->tables[level - 1][walker->indices[level - 1]] =
                    phys_create_pte(phys_addr) | PTE_LARGE_FLAGS(level) | flags;

                ptwalker_result =
                    ptwalker_next_with_options(walker,
                                               level,
                                               /*alloc_parents=*/false,
                                               /*alloc_level=*/false,
                                               /*should_ref=*/false,
                                               alloc_pgtable_cb_info,
                                               free_pgtable_cb_info);

                phys_addr += largepage_size;
                if (phys_addr == phys_end) {
                    *phys_addr_in = phys_addr;
                    return MAP_CONTINUE;
                }
            } while (phys_addr + largepage_size <= phys_end);
        }
    } else {
        if (is_overwrite) {
            do {
                if (ptwalker_result != E_PT_WALKER_OK) {
                    panic("mm: failed to setup kernel pagemap, result=%d\n",
                          ptwalker_result);
                }

                pte_t *const pte =
                    &walker->tables[level - 1][walker->indices[level - 1]];

                if (pte_is_present(*pte)) {
                    // TODO:
                }

                *pte =
                    phys_create_pte(phys_addr) | PTE_LARGE_FLAGS(level) | flags;
                ptwalker_result =
                    ptwalker_next_with_options(walker,
                                               level,
                                               /*alloc_parents=*/false,
                                               /*alloc_level=*/false,
                                               /*should_ref=*/false,
                                               alloc_pgtable_cb_info,
                                               free_pgtable_cb_info);

                phys_addr += largepage_size;
                if (phys_addr == phys_end) {
                    *phys_addr_in = phys_addr;
                    return MAP_DONE;
                }

                if (walker->indices[level - 1] == 0) {
                    *phys_addr_in = phys_addr;
                    return MAP_RESTART;
                }
            } while (phys_addr + largepage_size <= phys_end);
        } else {
            do {
                if (ptwalker_result != E_PT_WALKER_OK) {
                    panic("mm: failed to setup kernel pagemap, result=%d\n",
                          ptwalker_result);
                }

                walker->tables[level - 1][walker->indices[level - 1]] =
                    phys_create_pte(phys_addr) | PTE_LARGE_FLAGS(level) | flags;

                ptwalker_result =
                    ptwalker_next_with_options(walker,
                                               level,
                                               /*alloc_parents=*/false,
                                               /*alloc_level=*/false,
                                               /*should_ref=*/false,
                                               alloc_pgtable_cb_info,
                                               free_pgtable_cb_info);

                phys_addr += largepage_size;
                if (phys_addr == phys_end) {
                    *phys_addr_in = phys_addr;
                    return MAP_DONE;
                }

                if (walker->indices[level - 1] == 0) {
                    *phys_addr_in = phys_addr;
                    return MAP_RESTART;
                }
            } while (phys_addr + largepage_size <= phys_end);
        }
    }

    *phys_addr_in = phys_addr;
    return MAP_CONTINUE;
}

bool
vmap_at(const struct pagemap *const pagemap,
        const struct range phys_range,
        const uint64_t virt_addr,
        const uint64_t pte_flags,
        const bool should_ref,
        const bool is_overwrite,
        const uint8_t supports_largepage_at_level_mask)
{
    struct pt_walker walker;
    ptwalker_default_for_pagemap(&walker, pagemap, virt_addr);

    return
        vmap_at_with_ptwalker(&walker,
                              phys_range,
                              pte_flags,
                              should_ref,
                              is_overwrite,
                              supports_largepage_at_level_mask,
                              /*alloc_pgtable_cb_info=*/NULL,
                              /*free_pgtable_cb_info=*/NULL);
}

bool
vmap_at_with_ptwalker(struct pt_walker *const walker,
                      const struct range phys_range,
                      const uint64_t pte_flags,
                      const bool should_ref,
                      const bool is_overwrite,
                      const uint8_t supports_largepage_at_level_mask,
                      void *const alloc_pgtable_cb_info,
                      void *const free_pgtable_cb_info)
{
    if (range_empty(phys_range)) {
        printk(LOGLEVEL_WARN, "vmap_at(): phys-range is empty\n");
        return false;
    }

    uint64_t phys_end = 0;
    if (!range_get_end(phys_range, &phys_end)) {
        printk(LOGLEVEL_WARN,
               "vmap_at(): phys-range goes beyond end of address-space\n");
        return false;
    }

    uint64_t phys_addr = phys_range.front;
    if (!range_has_align(phys_range, PAGE_SIZE)) {
        printk(LOGLEVEL_WARN,
               "vmap_at(): phys-range isn't aligned to PAGE_SIZE\n");
        return false;
    }

    do {
    start:
        for (int8_t index = countof(LARGEPAGE_LEVELS) - 1;
             index >= 0;
             index--)
        {
            const pgt_level_t level = LARGEPAGE_LEVELS[index];
            if (!(supports_largepage_at_level_mask & (1ull << level))) {
                continue;
            }

            const enum map_result result =
                map_large_at_level(walker,
                                   &phys_addr,
                                   phys_end,
                                   level,
                                   pte_flags,
                                   should_ref,
                                   is_overwrite,
                                   alloc_pgtable_cb_info,
                                   free_pgtable_cb_info);

            switch (result) {
                case MAP_DONE:
                    return true;
                case MAP_CONTINUE:
                    break;
                case MAP_RESTART:
                    goto start;
            }
        }

        const enum map_result result =
            map_normal(walker,
                       &phys_addr,
                       phys_end,
                       pte_flags,
                       should_ref,
                       is_overwrite,
                       alloc_pgtable_cb_info,
                       free_pgtable_cb_info);

        switch (result) {
            case MAP_DONE:
                return true;
            case MAP_CONTINUE:
            case MAP_RESTART:
                break;
        }
    } while (phys_addr < phys_end);

    return true;
}