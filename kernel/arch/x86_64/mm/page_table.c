/*
 * kernel/arch/x86_64/mm/page_table.c
 * © suhas pai
 */

#include "asm/regs.h"
#include "lib/assert.h"

#include "mm/page_alloc.h"
#include "mm/walker.h"
#include "mm/tlb.h"

#include "page_table.h"

static struct page *
ptwalker_alloc_table_cb(struct pt_walker *const walker, void *const cb_info) {
	(void)walker;
	(void)cb_info;

	return alloc_table();
}

static void
ptwalker_free_table_cb(struct pt_walker *const walker,
					   struct page *const table,
					   void *const cb_info)
{
	(void)walker;
	pageop_free_table((struct pageop *)cb_info, table);
}

void
pageop_begin(struct pageop *const pageop,
             const uint64_t virt_addr,
             const uint8_t page_order)
{
	pageop->flush_range = range_create_empty();
	pageop->page_order = page_order;

	list_init(&pageop->delayed_free);
	ptwalker_create(&pageop->walker,
					/*root_phys=*/read_cr3(),
					virt_addr,
					ptwalker_alloc_table_cb,
					ptwalker_free_table_cb);
}

void pageop_free_table(struct pageop *const pageop, struct page *const table) {
	list_radd(&pageop->delayed_free, &table->pte.delayed_free_list);
}

void pageop_flush_range(struct pageop *const pageop, const struct range range) {
	if (pageop->flush_range.size == 0) {
		pageop->flush_range = range;
		return;
	}

	// TODO:
	pageop_end(pageop);
	pageop->flush_range = range;
}

void pageop_end(struct pageop *const pageop) {
	tlb_flush_pageop(pageop);
    pageop->flush_range = range_create_empty();
}

struct page *alloc_table() {
	return alloc_page(__ALLOC_TABLE | __ALLOC_ZERO);
}