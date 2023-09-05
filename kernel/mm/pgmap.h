/*
 * kernel/mm/pgmap.h
 * © suhas pai
 */

#pragma once
#include "pagemap.h"

struct pgmap_options {
    uint64_t pte_flags;

    void *alloc_pgtable_cb_info;
    void *free_pgtable_cb_info;

    uint8_t supports_largepage_at_level_mask;

    bool is_in_early : 1;
    bool is_overwrite : 1;
};

bool
pgmap_at(const struct pagemap *pagemap,
         struct range phys_range,
         uint64_t virt_addr,
         const struct pgmap_options *options);