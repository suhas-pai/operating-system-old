/*
 * kernel/arch/x86_64/mm/tlb.c
 * © suhas pai
 */

#include "asm/tlb.h"
#include "lib/assert.h"
#include "mm/page_alloc.h"

#include "tlb.h"

static
void tlb_flush_range(const struct range range, const uint64_t page_order) {
    uint64_t end = 0;
    assert(range_get_end(range, &end));

    for (uint64_t addr = range.front;
         addr < end;
         addr += PAGE_SIZE << page_order)
    {
        invlpg(addr);
    }
}

void tlb_flush_pageop(struct pageop *const pageop) {
    tlb_flush_range(pageop->flush_range, pageop->page_order);

    struct page *page = NULL;
    struct page *tmp = NULL;

    list_foreach_mut(page, tmp, &pageop->delayed_free, pte.delayed_free_list) {
        list_delete(&page->pte.delayed_free_list);
        free_page(page);
    }
}