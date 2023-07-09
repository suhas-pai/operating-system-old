/*
 * kernel/mm/page.c
 * © suhas pai
 */

#include <stdatomic.h>
#include "page.h"

void page_set_bit(struct page *const page, const enum struct_page_flags flag) {
    atomic_fetch_or_explicit(&page->flags, flag, memory_order_relaxed);
}

bool page_has_bit(struct page *const page, const enum struct_page_flags flag) {
    return atomic_load_explicit(&page->flags, memory_order_relaxed) & flag;
}

void
page_clear_bit(struct page *const page, const enum struct_page_flags flag) {
    atomic_fetch_and_explicit(&page->flags, ~flag, memory_order_relaxed);
}

uint8_t page_get_section(struct page *const page) {
    const uint32_t flags =
        atomic_load_explicit(&page->flags, memory_order_relaxed);

    return (flags >> SECTION_SHIFT) & SECTION_MASK;
}