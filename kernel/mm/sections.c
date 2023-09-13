/*
 * kernel/mm/sections.c
 * © suhas pai
 */

#include "lib/overflow.h"
#include "mm/page.h"

#include "boot.h"

__optimize(3) uint64_t phys_to_pfn(const uint64_t phys) {
    const struct mm_memmap *const begin = mm_get_usable_list();
    const struct mm_memmap *const end = begin + mm_get_usable_count();

    for (const struct mm_memmap *iter = begin; iter != end; iter++) {
        if (range_has_loc(iter->range, phys)) {
            return iter->pfn + ((phys - iter->range.front) >> PAGE_SHIFT);
        }
    }

    verify_not_reached();
}

__optimize(3) uint64_t page_to_phys(const struct page *const page) {
    const page_section_t section = page_get_section(page);
    const struct mm_memmap *const memmap = mm_get_usable_list() + section;
    const uint64_t relative_pfn =
        check_sub_assert(page_to_pfn(page), memmap->pfn);

    return memmap->range.front + (relative_pfn << PAGE_SHIFT);
}