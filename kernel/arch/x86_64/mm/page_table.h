/*
 * kernel/arch/x86_64/mm/page_table.h
 * © suhas pai
 */

#pragma once

#include "mm/pagemap.h"
#include "mm/walker.h"

struct pageop {
    struct pt_walker walker;
    struct range flush_range;
	struct list delayed_free;

	uint8_t page_order;
};

void
pagegop_begin(struct pageop *pageop, uint64_t virt_addr, uint8_t page_order);

void pageop_free_table(struct pageop *pageop, struct page *table);
void pageop_flush_range(struct pageop *pageop, struct range range);
void pageop_end(struct pageop *pageop);

struct page *alloc_table();