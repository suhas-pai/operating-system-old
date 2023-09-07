/*
 * kernel/arch/riscv64/mm/zone.c
 * © suhas pai
 */

#include "lib/size.h"
#include "mm/zone.h"

static struct page_zone zone_low4G = {
    .fallback_zone = NULL
};

static struct page_zone zone_default = {
    .fallback_zone = &zone_low4G
};

static struct page_zone zone_highmem = {
    .fallback_zone = &zone_default
};

__optimize(3) struct page_zone *page_to_zone(struct page *const page) {
    const uint64_t phys = page_to_phys(page);
    if (phys < gib(4)) {
        return &zone_low4G;
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
        return &zone_low4G;
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
