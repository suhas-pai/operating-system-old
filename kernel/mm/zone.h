/*
 * kernel/mm/zone.h
 * © suhas pai
 */

#pragma once
#include "page_alloc.h"

struct page_freelist {
    struct list page_list;
    uint64_t count;
};

struct page_zone {
    struct spinlock lock;
    const char *name;

    struct page_freelist freelist_list[MAX_ORDER];
    struct page_zone *const fallback_zone;
    uint64_t total_free;

    // Smallest non-empty order;
    uint8_t min_order;
};

struct page_zone *page_zone_iterstart();
struct page_zone *page_zone_iternext(struct page_zone *prev);
struct page_zone *page_alloc_flags_to_zone(uint64_t flags);

struct page;

struct page_zone *page_to_zone(const struct page *page);
struct page_zone *phys_to_zone(uint64_t phys);

#define for_each_page_zone(zone) \
    for (__auto_type zone = page_zone_iterstart(); \
         zone != NULL;                             \
         zone = page_zone_iternext(zone))
