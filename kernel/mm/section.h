/*
 * kernel/mm/section.h
 * © suhas pai
 */

#pragma once

#include "lib/adt/bitmap.h"
#include "mm_types.h"

struct mm_section {
    struct range range;
    uint64_t pfn;
};

struct mm_section *mm_get_usable_list();
