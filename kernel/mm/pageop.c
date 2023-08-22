/*
 * kernel/mm/pageop.c
 * © suhas pai
 */

#if defined(__x86_64__)
    #include "asm/regs.h"
#endif /* defined(__x86_64__) */

#include "mm/pageop.h"
#include "mm/page_alloc.h"

#if defined(__x86_64__)
    #include "mm/tlb.h"
#endif /* defined(__x86_64__) */

void pageop_init(struct pageop *const pageop) {
    pageop->flush_range = range_create_empty();
    list_init(&pageop->delayed_free);
}

void pageop_free_table(struct pageop *const pageop, struct page *const page) {
    list_radd(&pageop->delayed_free, &page->table.delayed_free_list);
}

void pageop_flush(struct pageop *const pageop, const uint64_t addr) {
    pageop_flush_range(pageop, range_create(addr, PAGE_SIZE));
}

void pageop_flush_range(struct pageop *const pageop, const struct range range) {
    if (pageop->flush_range.size == 0) {
        pageop->flush_range = range;
        return;
    }

    pageop_finish(pageop);
    pageop->flush_range = range;
}

void pageop_finish(struct pageop *const pageop) {
#if defined(__x86_64__)
    tlb_flush_pageop(pageop);
#elif defined(__riscv64)
    asm volatile ("fence.i" ::: "memory");
#endif /* defined(__x86_64__) */

    pageop->flush_range = range_create_empty();
}

struct page *alloc_table() {
    return alloc_page(__ALLOC_TABLE | __ALLOC_ZERO);
}