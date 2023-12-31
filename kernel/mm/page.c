/*
 * kernel/mm/page.c
 * © suhas pai
 */

#include <stdatomic.h>
#include "lib/memory.h"
#include "lib/overflow.h"

#include "page.h"

#if defined(__riscv)
    // FIXME: 64 is the cache-block size in qemu. This should instead be read
    // from the dtb
    #define CBO_SIZE 64
#endif /* defined(__riscv) */

__optimize(3) void zero_page(void *page) {
#if defined(__x86_64__)
    uint64_t count = PAGE_SIZE / sizeof(uint64_t);
    asm volatile ("cld;\n"
                  "rep stosq"
                  : "+D"(page), "+c" (count) : "a"(0) : "memory");
#elif defined(__aarch64__)
    const uint64_t *const end = page + PAGE_SIZE;
    for (uint64_t *iter = page; iter != end; iter += 32) {
        asm volatile ("stp xzr, xzr, [%0]" :: "r"(iter + 0));
        asm volatile ("stp xzr, xzr, [%0]" :: "r"(iter + 2));
        asm volatile ("stp xzr, xzr, [%0]" :: "r"(iter + 4));
        asm volatile ("stp xzr, xzr, [%0]" :: "r"(iter + 6));
        asm volatile ("stp xzr, xzr, [%0]" :: "r"(iter + 8));
        asm volatile ("stp xzr, xzr, [%0]" :: "r"(iter + 10));
        asm volatile ("stp xzr, xzr, [%0]" :: "r"(iter + 12));
        asm volatile ("stp xzr, xzr, [%0]" :: "r"(iter + 14));
        asm volatile ("stp xzr, xzr, [%0]" :: "r"(iter + 16));
        asm volatile ("stp xzr, xzr, [%0]" :: "r"(iter + 18));
        asm volatile ("stp xzr, xzr, [%0]" :: "r"(iter + 20));
        asm volatile ("stp xzr, xzr, [%0]" :: "r"(iter + 22));
        asm volatile ("stp xzr, xzr, [%0]" :: "r"(iter + 24));
        asm volatile ("stp xzr, xzr, [%0]" :: "r"(iter + 26));
        asm volatile ("stp xzr, xzr, [%0]" :: "r"(iter + 28));
        asm volatile ("stp xzr, xzr, [%0]" :: "r"(iter + 30));
    }
#elif defined(__riscv64)
    const void *const end = page + PAGE_SIZE;
    for (void *iter = page; iter != end; iter += (CBO_SIZE * 8)) {
        asm volatile ("cbo.zero (%0)" :: "r"(iter + (CBO_SIZE * 0)));
        asm volatile ("cbo.zero (%0)" :: "r"(iter + (CBO_SIZE * 1)));
        asm volatile ("cbo.zero (%0)" :: "r"(iter + (CBO_SIZE * 2)));
        asm volatile ("cbo.zero (%0)" :: "r"(iter + (CBO_SIZE * 3)));
        asm volatile ("cbo.zero (%0)" :: "r"(iter + (CBO_SIZE * 4)));
        asm volatile ("cbo.zero (%0)" :: "r"(iter + (CBO_SIZE * 5)));
        asm volatile ("cbo.zero (%0)" :: "r"(iter + (CBO_SIZE * 6)));
        asm volatile ("cbo.zero (%0)" :: "r"(iter + (CBO_SIZE * 7)));
    }
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
#elif defined(__aarch64__)
    const uint64_t *const end = page + full_size;
    for (uint64_t *iter = page; iter != end; iter += 32) {
        asm volatile ("stp xzr, xzr, [%0]" :: "r"(iter + 0));
        asm volatile ("stp xzr, xzr, [%0]" :: "r"(iter + 2));
        asm volatile ("stp xzr, xzr, [%0]" :: "r"(iter + 4));
        asm volatile ("stp xzr, xzr, [%0]" :: "r"(iter + 6));
        asm volatile ("stp xzr, xzr, [%0]" :: "r"(iter + 8));
        asm volatile ("stp xzr, xzr, [%0]" :: "r"(iter + 10));
        asm volatile ("stp xzr, xzr, [%0]" :: "r"(iter + 12));
        asm volatile ("stp xzr, xzr, [%0]" :: "r"(iter + 14));
        asm volatile ("stp xzr, xzr, [%0]" :: "r"(iter + 16));
        asm volatile ("stp xzr, xzr, [%0]" :: "r"(iter + 18));
        asm volatile ("stp xzr, xzr, [%0]" :: "r"(iter + 20));
        asm volatile ("stp xzr, xzr, [%0]" :: "r"(iter + 22));
        asm volatile ("stp xzr, xzr, [%0]" :: "r"(iter + 24));
        asm volatile ("stp xzr, xzr, [%0]" :: "r"(iter + 26));
        asm volatile ("stp xzr, xzr, [%0]" :: "r"(iter + 28));
        asm volatile ("stp xzr, xzr, [%0]" :: "r"(iter + 30));
    }
#elif defined(__riscv64)
    const void *const end = page + full_size;
    for (void *iter = page; iter != end; iter += (CBO_SIZE * 8)) {
        asm volatile ("cbo.zero (%0)" :: "r"(iter + (CBO_SIZE * 0)));
        asm volatile ("cbo.zero (%0)" :: "r"(iter + (CBO_SIZE * 1)));
        asm volatile ("cbo.zero (%0)" :: "r"(iter + (CBO_SIZE * 2)));
        asm volatile ("cbo.zero (%0)" :: "r"(iter + (CBO_SIZE * 3)));
        asm volatile ("cbo.zero (%0)" :: "r"(iter + (CBO_SIZE * 4)));
        asm volatile ("cbo.zero (%0)" :: "r"(iter + (CBO_SIZE * 5)));
        asm volatile ("cbo.zero (%0)" :: "r"(iter + (CBO_SIZE * 6)));
        asm volatile ("cbo.zero (%0)" :: "r"(iter + (CBO_SIZE * 7)));
    }
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
void page_set_flag(struct page *const page, const enum struct_page_flags flag) {
    atomic_fetch_or_explicit(&page->flags, flag, memory_order_relaxed);
}

__optimize(3) bool
page_has_flag(const struct page *const page,
              const enum struct_page_flags flag)
{
    return page_get_flags(page) & flag;
}

__optimize(3) void
page_clear_flag(struct page *const page, const enum struct_page_flags flag) {
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
struct page_section *page_to_section(const struct page *const page) {
    return &mm_get_page_section_list()[page->section];
}