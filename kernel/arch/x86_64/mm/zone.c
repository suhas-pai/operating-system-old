/*
 * kernel/arch/x86_64/mm/zone.c
 * © suhas pai
 */

#include "kernel/mm/page_alloc.h"
#include "lib/size.h"

#include "mm/page.h"
#include "mm/zone.h"

static struct page_zone zone_low_16mib = {
    .fallback_zone = NULL
};

static struct page_zone zone_default = {
    .fallback_zone = &zone_low_16mib
};

static struct page_zone zone_high_896gib = {
    .fallback_zone = &zone_default
};

struct page_zone *page_to_zone(struct page *const page) {
    const uint64_t phys = page_to_phys(page);
    if (phys <= mib(16)) {
        return &zone_low_16mib;
    }

    if (phys >= gib(896)) {
        return &zone_high_896gib;
    }

    return &zone_default;
}

struct page_zone *page_zone_iterstart() {
    return &zone_high_896gib;
}

struct page_zone *page_zone_iternext(struct page_zone *const zone) {
    return zone->fallback_zone;
}

struct page_zone *page_alloc_flags_to_zone(const uint64_t flags) {
    if (flags & __ALLOC_HIGHMEM) {
        return &zone_high_896gib;
    }

    if (flags & __ALLOC_LOWMEM) {
        return &zone_low_16mib;
    }

    return &zone_default;
}

void pagezones_init() {
    for_each_page_zone (zone) {
        for (uint8_t i = 0; i != MAX_ORDER; i++) {
            list_init(&zone->freelist_list[i].pages);

            zone->freelist_list[i].count = 0;
            zone->freelist_list[i].order = i;
        }
    }
}