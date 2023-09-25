/*
 * kernel/mm/page.c
 * © suhas pai
 */

#include <stdatomic.h>
#include "lib/memory.h"

#include "page.h"

void zero_page(void *const page) {
    memset_64(page, PAGE_SIZE / sizeof(uint64_t), 0);
}

void zero_multiple_pages(void *const page, const uint64_t count) {
    const uint64_t full_size = check_mul_assert(PAGE_SIZE, count);
    memset_64(page, full_size / sizeof(uint64_t), 0);
}

__optimize(3) uint32_t page_get_flags(const struct page *const page) {
    return atomic_load_explicit(&page->flags, memory_order_relaxed);
}

__optimize(3)
void page_set_bit(struct page *const page, const enum struct_page_flags flag) {
    atomic_fetch_or_explicit(&page->flags, flag, memory_order_relaxed);
}

__optimize(3) bool
page_has_bit(const struct page *const page, const enum struct_page_flags flag) {
    return page_get_flags(page) & flag;
}

__optimize(3) void
page_clear_bit(struct page *const page, const enum struct_page_flags flag) {
    atomic_fetch_and_explicit(&page->flags, ~flag, memory_order_relaxed);
}

__optimize(3) page_section_t page_get_section(const struct page *const page) {
    return (page_get_flags(page) >> SECTION_SHIFT) & SECTION_MASK;
}

__optimize(3) struct mm_section *page_to_mm_section(const struct page *page) {
    return mm_get_usable_list() + page_get_section(page);
}