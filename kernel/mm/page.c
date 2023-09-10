/*
 * kernel/mm/page.c
 * © suhas pai
 */

#include <stdatomic.h>
#include "page.h"

__optimize(3) uint32_t page_get_flags(const struct page *const page) {
    return atomic_load_explicit(&page->flags, memory_order_relaxed);
}

__optimize(3)
void page_set_bit(struct page *const page, const enum struct_page_flags flag) {
    atomic_fetch_or_explicit(&page->flags, flag, memory_order_relaxed);
}

__optimize(3) bool
page_has_bit(const struct page *const page, const enum struct_page_flags flag) {
    return (page_get_flags(page) & flag) != 0;
}

__optimize(3) void
page_clear_bit(struct page *const page, const enum struct_page_flags flag) {
    atomic_fetch_and_explicit(&page->flags, ~flag, memory_order_relaxed);
}

__optimize(3) page_section_t page_get_section(const struct page *const page) {
    return (page_get_flags(page) >> SECTION_SHIFT) & SECTION_MASK;
}