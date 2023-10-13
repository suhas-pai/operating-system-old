/*
 * kernel/mm/page.c
 * © suhas pai
 */

#include <stdatomic.h>
#include "lib/memory.h"

#include "page.h"

__optimize(3) void zero_page(void *page) {
#if defined(__x86_64__)
    uint64_t count = PAGE_SIZE / sizeof(uint64_t);
    asm volatile ("cld;\n"
                  "rep stosq"
                  : "+D"(page), "+c" (count) : "a"(0) : "memory");
#else
    const uint64_t *const end = page + PAGE_SIZE;
    for (uint64_t *iter = page; iter != end; iter++) {
        *iter = 0;
    }
#endif /* defined(__x86_64__) */
}

__optimize(3) void zero_multiple_pages(void *page, const uint64_t count) {
    const uint64_t full_size = check_mul_assert(PAGE_SIZE, count);
#if defined(__x86_64__)
    uint64_t qword_count = full_size / sizeof(uint64_t);
    asm volatile ("cld;\n"
                  "rep stosq"
                  : "+D"(page), "+c" (qword_count) : "a"(0) : "memory");
#else
    const uint64_t *const end = page + full_size;
    for (uint64_t *iter = page; iter != end; iter++) {
        *iter = 0;
    }
#endif /* defined(__x86_64__) */
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

__optimize(3) enum page_state page_get_state(const struct page *const page) {
    return atomic_load_explicit(&page->state, memory_order_relaxed);
}

__optimize(3)
void page_set_state(struct page *const page, const enum page_state state) {
    atomic_store_explicit(&page->state, state, memory_order_relaxed);
}

__optimize(3)
struct mm_section *page_to_mm_section(const struct page *const page) {
    return &mm_get_usable_list()[page->section];
}