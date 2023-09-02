/*
 * kernel/mm/page_zone.h
 * © suhas pai
 */

#pragma once

#include "cpu/spinlock.h"
#include "lib/list.h"

#include "page.h"

#define MAX_ORDER 11

#define alloc_page(flags) alloc_pages((flags), /*order=*/0)
#define free_page(page) free_pages((page), /*order=*/0)

enum page_alloc_flags {
    __ALLOC_ZERO = 1 << 0,
    __ALLOC_TABLE = 1 << 1,
    __ALLOC_SLAB_HEAD = 1 << 2,
    __ALLOC_LOW4G = 1 << 3,
    __ALLOC_HIGHMEM = 1 << 4,
};

struct page *alloc_pages(uint64_t alloc_flags, uint8_t order);
struct page_zone;

// free_pages will call zero-out the page. Call page_to_zone() and
// free_pages_to_zone() directly to avoid this.

void free_pages(struct page *page, uint8_t order);

void
free_pages_to_zone(struct page *page,
                   struct page_zone *zone,
                   uint8_t order);
