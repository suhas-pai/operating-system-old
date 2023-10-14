/*
 * kernel/mm/page_alloc.h
 * © suhas pai
 */

#pragma once

#include "cpu/spinlock.h"
#include "lib/list.h"

#include "page.h"
#include "pageop.h"

#define alloc_page(state, flags) alloc_pages((state), (flags), /*order=*/0)
#define free_page(page) free_pages((page), /*order=*/0)

enum page_alloc_flags {
    __ALLOC_ZERO = 1 << 0,
};

uint64_t buddy_of(uint64_t page_pfn, uint8_t order);

// free_pages will call zero-out the page. Call page_to_zone() and
// free_pages_to_zone() directly to avoid this.

void free_pages(struct page *page, uint8_t order);
void free_large_page(struct page *page);

struct page *deref_page(struct page *page, struct pageop *pageop);

__malloclike __malloc_dealloc(free_pages, 1)
struct page *
alloc_pages(enum page_state state, uint64_t alloc_flags, uint8_t order);

struct page_zone;

__malloclike __malloc_dealloc(free_pages, 1)
struct page *
alloc_pages_in_zone(struct page_zone *zone,
                    enum page_state state,
                    uint64_t alloc_flags,
                    uint8_t order,
                    bool fallback);

__malloclike __malloc_dealloc(free_pages, 1)
struct page *alloc_large_page(uint64_t alloc_flags, pgt_level_t level);

__malloclike __malloc_dealloc(free_pages, 1)
struct page *
alloc_large_page_in_zone(struct page_zone *zone,
                         uint64_t alloc_flags,
                         pgt_level_t level,
                         bool fallback);

void
free_pages_to_zone(struct page *page,
                   struct page_zone *zone,
                   uint8_t order);
