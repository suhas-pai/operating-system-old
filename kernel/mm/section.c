/*
 * kernel/mm/sections.c
 * © suhas pai
 */

#include "lib/overflow.h"
#include "mm/page.h"

#include "boot.h"

__optimize(3) uint64_t phys_to_pfn(const uint64_t phys) {
    const struct mm_section *const begin = mm_get_usable_list();
    const struct mm_section *const end = begin + mm_get_usable_count();

    for (const struct mm_section *iter = begin; iter != end; iter++) {
        if (range_has_loc(iter->range, phys)) {
            return iter->pfn + ((phys - iter->range.front) >> PAGE_SHIFT);
        }
    }

    verify_not_reached();
}

__optimize(3) uint64_t page_to_phys(const struct page *const page) {
    const struct mm_section *const memmap = page_to_mm_section(page);

    const uint64_t page_pfn = page_to_pfn(page);
    const uint64_t relative_pfn = check_sub_assert(page_pfn, memmap->pfn);

    return memmap->range.front + (relative_pfn << PAGE_SHIFT);
}