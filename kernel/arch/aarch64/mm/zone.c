/*
 * kernel/arch/aarch64/mm/zone.c
 * © suhas pai
 */

#include "mm/zone.h"

static struct page_zone zone_lowmem = {
    .fallback_zone = NULL
};

static struct page_zone zone_default = {
    .fallback_zone = &zone_lowmem
};

static struct page_zone zone_highmem = {
    .fallback_zone = &zone_highmem
};

struct page_zone *page_to_zone(struct page *const page) {
    (void)page;
    return &zone_default;
}

struct page_zone *page_zone_iterstart() {
    return &zone_highmem;
}

struct page_zone *page_zone_iternext(struct page_zone *const zone) {
    return zone->fallback_zone;
}

struct page_zone *page_alloc_flags_to_zone(const uint64_t flags) {
    if (flags & __ALLOC_HIGHMEM) {
        return &zone_highmem;
    }

    if (flags & __ALLOC_LOWMEM) {
        return &zone_lowmem;
    }

    return &zone_default;
}

void pagezones_init() {
    for_each_page_zone (zone) {
        for (uint8_t i = 0; i != MAX_ORDER; i++) {
            list_init(&zone->freelist_list[i].pages);
        }
    }
}
