/*
 * kernel/mm/zone.h
 * © suhas pai
 */

#pragma once
#include "page_alloc.h"

struct page_freelist {
    struct list page_list;

    uint8_t order;
    uint64_t count;
};

struct page_zone {
    struct spinlock lock;
    struct page_freelist freelist_list[MAX_ORDER];
    struct page_zone *const fallback_zone;

    // Smallest non-empty order;
    _Atomic uint8_t min_order;
};

struct page_zone *page_zone_iterstart();
struct page_zone *page_zone_iternext(struct page_zone *prev);
struct page_zone *page_alloc_flags_to_zone(uint64_t flags);

struct page;
struct page_zone *page_to_zone(struct page *const page);

#define for_each_page_zone(zone) \
    for (__auto_type zone = page_zone_iterstart(); \
         zone != NULL;                             \
         zone = page_zone_iternext(zone))
