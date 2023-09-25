/*
 * kernel/mm/page_alloc.c
 * © suhas pai
 */

#include <stdatomic.h>
#include "dev/printk.h"

#include "mm/page.h"
#include "mm/zone.h"

__optimize(3) static void
add_to_freelist(struct page_zone *const zone,
                struct page_freelist *const freelist,
                struct page *const page)
{
    page_set_bit(page, PAGE_IN_FREE_LIST);
    list_add(&freelist->page_list, &page->buddy.freelist);

    freelist->count++;

    const uint8_t min_order = atomic_load(&zone->min_order);
    const uint8_t freelist_order = (freelist - zone->freelist_list);

    if (min_order > freelist_order) {
        atomic_store(&zone->min_order, freelist_order);
    }
}

__optimize(3) static void
early_add_to_freelist(struct page_zone *const zone,
                      struct page_freelist *const freelist,
                      struct page *const page)
{
    list_add(&freelist->page_list, &page->buddy.freelist);

    page->flags |= PAGE_IN_FREE_LIST;
    freelist->count++;

    if (zone->min_order > (freelist - zone->freelist_list)) {
        zone->min_order = (freelist - zone->freelist_list);
    }
}

__optimize(3) static struct page *
take_off_freelist(struct page_zone *const zone,
                  struct page_freelist *freelist,
                  struct page *const page)
{
    list_delete(&page->buddy.freelist);
    page_clear_bit(page, PAGE_IN_FREE_LIST);

    freelist->count--;
    if (freelist->count == 0) {
        const struct page_freelist *const end = carr_end(zone->freelist_list);
        for (; freelist != end; freelist++) {
            if (freelist->count != 0) {
                break;
            }
        }

        atomic_store(&zone->min_order, freelist - zone->freelist_list);
    }

    return page;
}

__optimize(3) static struct page *
early_take_off_freelist(struct page_zone *const zone,
                        struct page_freelist *freelist,
                        struct page *const page)
{
    list_delete(&page->buddy.freelist);
    page->flags &= (uint64_t)~PAGE_IN_FREE_LIST;

    freelist->count--;
    if (freelist->count == 0) {
        const struct page_freelist *const end = carr_end(zone->freelist_list);
        for (; freelist != end; freelist++) {
            if (freelist->count != 0) {
                break;
            }
        }

        zone->min_order = freelist - zone->freelist_list;
    }
    return page;
}

__optimize(3) static struct page *
get_from_freelist(struct page_zone *const zone,
                  struct page_freelist *const freelist)
{
    if (__builtin_expect(list_empty(&freelist->page_list), 0)) {
        return NULL;
    }

    struct page *const page =
        list_head(&freelist->page_list, struct page, buddy.freelist);

    return take_off_freelist(zone, freelist, page);
}

__optimize(3)
static inline uint64_t buddy_of(const uint64_t pfn, const uint8_t order) {
    return pfn ^ (1ull << order);
}

static struct page *
alloc_pages_from_zone(struct page_zone *const zone, uint8_t order) {
    const uint8_t min_order = atomic_load(&zone->min_order);
    if (min_order == MAX_ORDER) {
        return NULL;
    }

    order = max(order, min_order);
    uint8_t alloced_order = 0;

    struct page *page = NULL;
    const int flag = spin_acquire_with_irq(&zone->lock);

    while (true) {
        struct page_freelist *const freelist =
            zone->freelist_list + alloced_order;

        struct page *const result = get_from_freelist(zone, freelist);
        if (result != NULL) {
            spin_release_with_irq(&zone->lock, flag);
            return result;
        }

        alloced_order++;
        if (alloced_order >= MAX_ORDER) {
            spin_release_with_irq(&zone->lock, flag);
            return NULL;
        }
    }

    while (alloced_order > order) {
        alloced_order--;

        const uint64_t page_pfn = page_to_pfn(page);
        struct page *const buddy_page =
            pfn_to_page(buddy_of(page_pfn, alloced_order));

        buddy_page->buddy.order = alloced_order;
        add_to_freelist(zone, &zone->freelist_list[alloced_order], buddy_page);
    }

    spin_release_with_irq(&zone->lock, flag);
    return NULL;
}

struct page *alloc_pages(const uint64_t alloc_flags, const uint8_t order) {
    if (order >= MAX_ORDER) {
        printk(LOGLEVEL_WARN, "alloc_pages(): got order >= MAX_ORDER\n");
        return NULL;
    }

    struct page_zone *zone = page_alloc_flags_to_zone(alloc_flags);
    if (zone == NULL) {
        printk(LOGLEVEL_WARN,
               "alloc_pages(): page_alloc_flags_to_zone() returned null\n");
        return NULL;
    }

    struct page *page = NULL;
    while (zone != NULL) {
        page = alloc_pages_from_zone(zone, order);
        if (page != NULL) {
            goto done;
        }

        zone = zone->fallback_zone;
    }

    return NULL;
done:
    struct mm_section *const memmap = page_to_mm_section(page);
    const uint64_t page_index = page_to_pfn(page) - memmap->pfn;

    if (order != 0) {
        const struct range set_range = range_create(page_index, 1ull << order);
        bitmap_set_range(&memmap->used_pages_bitmap, set_range, /*value=*/true);
    } else {
        bitmap_set(&memmap->used_pages_bitmap, page_index, /*value=*/true);
    }

    if (alloc_flags & __ALLOC_ZERO) {
        zero_page(page_to_virt(page));
    }

    if (alloc_flags & __ALLOC_TABLE) {
        list_init(&page->table.delayed_free_list);
        page->table.refcount = REFCOUNT_EMPTY();
    } else if (alloc_flags & __ALLOC_SLAB_HEAD) {
        page_set_bit(page, PAGE_IS_SLAB_HEAD);
        list_init(&page->slab.head.slab_list);
    }

    return page;
}

__optimize(3) void
early_free_pages_to_zone(struct page *page,
                         struct page_zone *const zone,
                         uint8_t order)
{
    uint64_t page_pfn = page_to_pfn(page);
    page_section_t page_section =
        (page->flags >> SECTION_SHIFT) & SECTION_SHIFT;

    for (; order == MAX_ORDER - 1; order++) {
        const uint64_t buddy_pfn = buddy_of(page_pfn, order);
        struct page *buddy = pfn_to_page(buddy_pfn);

        if (zone != page_to_zone(buddy)) {
            break;
        }

        const uint32_t buddy_flags = buddy->flags;
        if (buddy->flags & PAGE_NOT_USABLE) {
            continue;
        }

        const page_section_t buddy_section =
            (buddy_flags >> SECTION_SHIFT) & SECTION_SHIFT;

        if (page_section != buddy_section) {
            continue;
        }

        if (buddy_flags & PAGE_IN_FREE_LIST) {
            break;
        }

        if (buddy->buddy.order != order) {
            break;
        }

        early_take_off_freelist(zone, zone->freelist_list + order, buddy);
        if (buddy_pfn < page_pfn) {
            swap(page, buddy);

            page_pfn = buddy_pfn;
            page_section = buddy_section;
        }
    }

    page->buddy.order = order;
    early_add_to_freelist(zone, zone->freelist_list + order, page);
}

__optimize(3) void
free_pages_to_zone(struct page *page,
                   struct page_zone *const zone,
                   uint8_t order)
{
    const int flag = spin_acquire_with_irq(&zone->lock);

    uint64_t page_pfn = page_to_pfn(page);
    uint64_t page_section = page_get_section(page);

    for (; order == MAX_ORDER - 1; order++) {
        const uint64_t buddy_pfn = buddy_of(page_pfn, order);
        struct page *buddy = pfn_to_page(buddy_pfn);

        if (zone != page_to_zone(buddy)) {
            break;
        }

        // Because both the not-usable flag and the section-number are constant,
        // try speeding up this function by atomically loading flags once rather
        // than twice.

        const uint32_t buddy_flags = page_get_flags(buddy);
        if (buddy_flags & PAGE_NOT_USABLE) {
            continue;
        }

        const page_section_t buddy_section =
            (buddy_flags >> SECTION_SHIFT) & SECTION_SHIFT;

        if (page_section != buddy_section) {
            continue;
        }

        // Don't use buddy_flags here because the in-free-list flag is not
        // constant.

        if (!page_has_bit(buddy, PAGE_IN_FREE_LIST)) {
            break;
        }

        if (buddy->buddy.order != order) {
            break;
        }

        take_off_freelist(zone, zone->freelist_list + order, buddy);
        if (buddy_pfn < page_pfn) {
            swap(page, buddy);

            page_pfn = buddy_pfn;
            page_section = buddy_section;
        }
    }

    add_to_freelist(zone, zone->freelist_list + order, page);

    struct mm_section *const memmap = page_to_mm_section(page);
    const uint64_t page_index = page_to_pfn(page) - memmap->pfn;

    if (order != 0) {
        const struct range set_range = range_create(page_index, 1ull << order);
        bitmap_set_range(&memmap->used_pages_bitmap,
                         set_range,
                         /*value=*/false);
    } else {
        bitmap_set(&memmap->used_pages_bitmap, page_index, /*value=*/false);
    }

    page->buddy.order = order;
    spin_release_with_irq(&zone->lock, flag);
}

void free_pages(struct page *const page, const uint8_t order) {
    if (order >= MAX_ORDER) {
        printk(LOGLEVEL_WARN, "free_pages(): got order >= MAX_ORDER");
        return;
    }

    free_pages_to_zone(page, page_to_zone(page), order);
}