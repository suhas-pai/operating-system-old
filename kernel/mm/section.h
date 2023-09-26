/*
 * kernel/mm/section.h
 * © suhas pai
 */

#pragma once

#include "lib/adt/bitmap.h"
#include "lib/list.h"

#include "mm/memmap.h"
#include "mm_types.h"

struct mm_section {
    struct range range;
    uint64_t pfn;

    /*
     * Store memmap large enough to store a large page in a linked-list for each
     * supported large page level.
     */

    struct list largepage_list[countof(LARGEPAGE_LEVELS)];
    struct bitmap used_pages_bitmap;
};

struct mm_section *mm_get_usable_list();
struct list *mm_get_largepage_memmap_list(pgt_level_t level);
