/*
 * kernel/mm/vmap.h
 * © suhas pai
 */

#pragma once

#include "lib/adt/range.h"
#include "pagemap.h"

bool
vmap_at(const struct pagemap *pagemap,
        struct range phys_range,
        const uint64_t pte_flags,
        uint64_t virt_addr,
        bool should_ref,
        bool is_overwrite,
        uint8_t supports_largepage_at_level_mask);

bool
vmap_at_with_ptwalker(struct pt_walker *const walker,
                      struct range phys_range,
                      const uint64_t pte_flags,
                      bool should_ref,
                      bool is_overwrite,
                      uint8_t supports_largepage_at_level_mask,
                      void *const alloc_pgtable_cb_info,
                      void *const free_pgtable_cb_info);