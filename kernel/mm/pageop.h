/*
 * kernel/mm/pageop.h
 * © suhas pai
 */

#pragma once

#include "lib/adt/range.h"
#include "lib/list.h"

#include "mm_types.h"

struct pagemap;
struct pageop {
    struct pagemap *pagemap;
    struct range flush_range;
    struct list delayed_free;
};

#define PAGEOP_INIT(name) \
    ((struct pageop){ \
        .flush_range = range_create_empty(), \
        .delayed_free = LIST_INIT(name.delayed_free) \
    })

void pageop_init(struct pageop *pageop);
void pageop_flush_pte(struct pageop *pageop, pte_t *pte, pgt_level_t level);
void pageop_flush_address(struct pageop *pageop, uint64_t virt);

void pageop_finish(struct pageop *pageop);