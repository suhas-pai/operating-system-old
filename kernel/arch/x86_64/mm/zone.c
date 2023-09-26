/*
 * kernel/arch/x86_64/mm/zone.c
 * © suhas pai
 */

#include "kernel/mm/page_alloc.h"
#include "lib/size.h"

#include "mm/page.h"
#include "mm/zone.h"

static struct page_zone zone_low4g = {
    .lock = SPINLOCK_INIT(),
    .name = "low4g",

    .freelist_list = {},
    .fallback_zone = NULL,

    .min_order = MAX_ORDER,
};

static struct page_zone zone_default = {
    .lock = SPINLOCK_INIT(),
    .name = "default",

    .freelist_list = {},
    .fallback_zone = &zone_low4g,

    .min_order = MAX_ORDER,
};

static struct page_zone zone_highmem = {
    .lock = SPINLOCK_INIT(),
    .name = "highmem",

    .freelist_list = {},
    .fallback_zone = &zone_default,

    .min_order = MAX_ORDER,
};

__optimize(3) struct page_zone *phys_to_zone(const uint64_t phys) {
    if (phys < gib(4)) {
        return &zone_low4g;
    }

    if (phys >= gib(896)) {
        return &zone_highmem;
    }

    return &zone_default;
}

__optimize(3) struct page_zone *page_zone_iterstart() {
    return &zone_highmem;
}

__optimize(3)
struct page_zone *page_zone_iternext(struct page_zone *const zone) {
    return zone->fallback_zone;
}

__optimize(3) struct page_zone *page_alloc_flags_to_zone(const uint64_t flags) {
    if (flags & __ALLOC_HIGHMEM) {
        return &zone_highmem;
    }

    if (flags & __ALLOC_LOW4G) {
        return &zone_low4g;
    }

    return &zone_default;
}

void pagezones_init() {
    for_each_page_zone (zone) {
        for (uint8_t i = 0; i != MAX_ORDER; i++) {
            list_init(&zone->freelist_list[i].page_list);

            zone->freelist_list[i].count = 0;
            zone->freelist_list[i].order = i;
        }
    }
}