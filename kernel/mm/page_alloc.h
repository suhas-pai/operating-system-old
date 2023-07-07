/*
 * kernel/mm/page_zone.h
 * © suhas pai
 */

#pragma once
#include <stdint.h>

#include "cpu/spinlock.h"
#include "lib/list.h"

// Support page sizes 4kib to 1gib
#define MAX_ORDER 11

#define alloc_page(flags) alloc_pages(/*order=*/0, (flags))
#define free_page(page) free_pages((page), /*order=*/0)

enum page_alloc_flags {
    PG_ALLOC_ZERO = 1 << 0,
    PG_ALLOC_PTE = 1 << 1,
    PG_ALLOC_SLAB_HEAD = 1 << 2,

    PG_ALLOC_LOWMEM = 1 << 3, // Lower 256 MiB
    PG_ALLOC_HIGHMEM = 1 << 4, // Upper 896 MiB
};

struct page *alloc_pages(uint8_t order, uint64_t alloc_flags);
void free_pages(struct page *page, uint8_t order);
