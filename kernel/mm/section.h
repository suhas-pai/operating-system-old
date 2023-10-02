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

    struct bitmap used_pages_bitmap;
};

struct mm_section *mm_get_usable_list();
