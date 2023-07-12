/*
 * kernel/mm/pageop.h
 * © suhas pai
 */

#pragma once

#include "lib/adt/range.h"
#include "mm/page.h"

struct pageop {
    struct range flush_range;
    struct list delayed_free;
};

void pageop_init(struct pageop *pageop);
void pageop_free_table(struct pageop *pageop, struct page *table);

void pageop_flush(struct pageop *pageop, uint64_t addr);
void pageop_flush_range(struct pageop *pageop, struct range range);

void pageop_finish(struct pageop *pageop);