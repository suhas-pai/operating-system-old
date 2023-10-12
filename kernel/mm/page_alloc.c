/*
 * kernel/mm/page_alloc.c
 * © suhas pai
 */

#include <stdatomic.h>

#include "dev/printk.h"
#include "lib/align.h"

#include "alloc.h"
#include "page.h"
#include "zone.h"

__optimize(3) static void
add_to_freelist(struct page_zone *const zone,
                struct page_freelist *const freelist,
                struct page *const page)
{
    page_set_state(page, PAGE_STATE_FREE_LIST);

    list_init(&page->buddy.freelist);
    list_add(&freelist->page_list, &page->buddy.freelist);

    freelist->count++;

    const uint8_t freelist_order = (freelist - zone->freelist_list);
    if (zone->min_order > freelist_order) {
        zone->min_order = freelist_order;
    }
}

__optimize(3) static void
early_add_to_freelist(struct page_zone *const zone,
                      struct page_freelist *const freelist,
                      struct page *const page)
{
    list_init(&page->buddy.freelist);
    list_add(&freelist->page_list, &page->buddy.freelist);

    page->state = PAGE_STATE_FREE_LIST;
    freelist->count++;

    const uint8_t freelist_order = (freelist - zone->freelist_list);
    if (zone->min_order > freelist_order) {
        zone->min_order = freelist_order;
    }
}

__optimize(3) static struct page *
take_off_freelist(struct page_zone *const zone,
                  struct page_freelist *freelist,
                  struct page *const page,
                  const enum page_state state)
{
    list_delete(&page->buddy.freelist);
    page_set_state(page, state);

    const uint8_t freelist_order = freelist - zone->freelist_list;

    freelist->count--;
    zone->total_free -= 1ull << freelist_order;

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
                  struct page_freelist *const freelist,
                  const enum page_state state)
{
    struct page *page =
        list_head(&freelist->page_list, struct page, buddy.freelist);

    return take_off_freelist(zone, freelist, page, state);
}

__optimize(3) uint64_t buddy_of(const uint64_t pfn, const uint8_t order) {
    return pfn ^ (1ull << order);
}

__optimize(3) static void
free_pages_to_zone_unlocked(struct page *page,
                            struct page_zone *const zone,
                            uint8_t order)
{
    uint64_t page_pfn = page_to_pfn(page);
    uint64_t page_section = page->section;

    for (; order < MAX_ORDER - 1; order++) {
        const uint64_t buddy_pfn = buddy_of(page_pfn, order);
        struct page *buddy = pfn_to_page(buddy_pfn);

        if (page_get_state(buddy) != PAGE_STATE_FREE_LIST) {
            break;
        }

        if (buddy->buddy.order != order) {
            break;
        }

        const page_section_t buddy_section = buddy->section;
        if (page_section != buddy_section) {
            break;
        }

        if (zone != page_to_zone(buddy)) {
            break;
        }

        struct page_freelist *const freelist = &zone->freelist_list[order];
        take_off_freelist(zone, freelist, buddy, PAGE_STATE_USED);

        if (buddy_pfn < page_pfn) {
            page = buddy;
            page_pfn = buddy_pfn;
            page_section = buddy_section;
        }
    }

    add_to_freelist(zone, &zone->freelist_list[order], page);

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
}

__optimize(3) void
free_range_of_pages(struct page *page,
                    struct page_zone *const zone,
                    const uint64_t amount)
{
    uint64_t avail = amount;
    uint64_t page_pfn = page_to_pfn(page);

    int8_t order = MAX_ORDER - 1;
    for (; order >= 0; order--) {
        if (avail >= (1ull << order)) {
            break;
        }
    }

    do {
        uint8_t max_free_order = (uint8_t)order;
        for (int8_t iorder = order; iorder >= 0; iorder--) {
            const uint64_t buddy_pfn = buddy_of(page_pfn, (uint8_t)iorder);
            if (buddy_pfn < page_pfn) {
                max_free_order = (uint8_t)iorder;
            }
        }

        free_pages_to_zone_unlocked(page, zone, max_free_order);

        const uint64_t page_count = 1ull << max_free_order;
        avail -= page_count;

        if (avail == 0) {
            break;
        }

        page += page_count;
        page_pfn += page_count;

        for (; order >= 0; order--) {
            if (avail >= (1ull << order)) {
                break;
            }
        }
    } while (true);
}

__optimize(3) static struct page *
get_large_from_freelist(struct page_zone *const zone,
                        struct page_freelist *const freelist,
                        const uint8_t largepage_order)
{
    const uint8_t freelist_order = freelist - zone->freelist_list;
    struct page *head =
        list_head(&freelist->page_list, struct page, buddy.freelist);

    struct page *page = head;
    uint64_t page_phys = page_to_phys(page);

    if (largepage_order != freelist_order) {
        take_off_freelist(zone, freelist, head, PAGE_STATE_LARGE_HEAD);
        if (!has_align(page_phys, PAGE_SIZE << largepage_order)) {
            uint64_t new_phys = 0;
            if (!align_up(page_phys, PAGE_SIZE << largepage_order, &new_phys)) {
                printk(LOGLEVEL_INFO,
                       "mm: alloc_large_page() failed to align page's physical "
                       "address to boundary at order %" PRIu8 "\n",
                       largepage_order);
                return NULL;
            }

            page += PAGE_COUNT(new_phys - page_phys);
            page_phys = new_phys;

            free_range_of_pages(head, zone, (uint64_t)(page - head));
            page_set_state(page, PAGE_STATE_LARGE_HEAD);
        }

        struct page *begin = page + (1ull << largepage_order);

        const struct page *const end = head + (1ull << freelist_order);
        const uint64_t count = (uint64_t)(end - begin);

        if (count != 0) {
            free_range_of_pages(begin, zone, count);
        }
    } else {
        const uint64_t align = PAGE_SIZE << largepage_order;
        do {
            if (has_align(page_phys, align)) {
                take_off_freelist(zone,
                                  freelist,
                                  head,
                                  PAGE_STATE_LARGE_HEAD);
                break;
            }

            head =
                container_of(head->buddy.freelist.next,
                             struct page,
                             buddy.freelist);

            if (&head->buddy.freelist == &freelist->page_list) {
                return NULL;
            }

            page = head;
            page_phys = page_to_phys(page);
        } while (true);
    }

    return page;
}

__optimize(3) static struct page *
alloc_pages_from_zone(struct page_zone *const zone,
                      const uint8_t order,
                      const enum page_state state)
{
    struct page *page = NULL;
    const int flag = spin_acquire_with_irq(&zone->lock);

    if (__builtin_expect(zone->total_free < (1ull << order), 0)) {
        spin_release_with_irq(&zone->lock, flag);
        return NULL;
    }

    // zone->min_order should never equal MAX_ORDER if zone->total_free is not 0
    uint8_t alloced_order = max(order, zone->min_order);
    while (true) {
        struct page_freelist *const freelist =
            &zone->freelist_list[alloced_order];

        page = get_from_freelist(zone, freelist, state);
        if (page != NULL) {
            break;
        }

        alloced_order++;
        if (alloced_order >= MAX_ORDER) {
            spin_release_with_irq(&zone->lock, flag);
            return NULL;
        }
    }

    const uint64_t page_pfn = page_to_pfn(page);
    while (alloced_order > order) {
        alloced_order--;
        struct page *const buddy_page =
            pfn_to_page(buddy_of(page_pfn, alloced_order));

        buddy_page->buddy.order = alloced_order;
        add_to_freelist(zone, &zone->freelist_list[alloced_order], buddy_page);
    }

    spin_release_with_irq(&zone->lock, flag);
    return page;
}

struct page *
setup_alloced_page(struct page *const page,
                   const enum page_state state,
                   const uint64_t alloc_flags,
                   const uint8_t order)
{
    struct mm_section *const memmap = page_to_mm_section(page);
    const uint64_t page_index = page_to_pfn(page) - memmap->pfn;

    if (order != 0) {
        const struct range set_range = range_create(page_index, 1ull << order);
        bitmap_set_range(&memmap->used_pages_bitmap, set_range, /*value=*/true);
    } else {
        bitmap_set(&memmap->used_pages_bitmap, page_index, /*value=*/true);
    }

    switch (state) {
        case PAGE_STATE_NOT_USABLE:
            verify_not_reached();
        case PAGE_STATE_USED:
            break;
        case PAGE_STATE_FREE_LIST:
            verify_not_reached();
        case PAGE_STATE_LRU_CACHE:
            verify_not_reached();
        case PAGE_STATE_SLAB_HEAD:
            zero_multiple_pages(page_to_virt(page), 1ull << order);
            list_init(&page->slab.head.slab_list);

            return page;
        case PAGE_STATE_TABLE:
            zero_page(page_to_virt(page));
            list_init(&page->table.delayed_free_list);

            page->table.refcount = REFCOUNT_EMPTY();
            return page;
        case PAGE_STATE_LARGE_HEAD: {
            const struct page *const end = page + (1ull << order);

            refcount_init(&page->largehead.refcount);
            refcount_init(&page->largehead.page_refcount);

            list_init(&page->largehead.delayed_free_list);
            for (struct page *iter = page + 1; iter != end; iter++) {
                page_set_state(iter, PAGE_STATE_LARGE_TAIL);
                refcount_init(&iter->largetail.refcount);

                iter->largetail.head = page;
            }

            break;
        }
        case PAGE_STATE_LARGE_TAIL:
            verify_not_reached();
    }

    const struct page *const end = page + (1ull << order);
    for (struct page *iter = page; iter != end; iter++) {
        list_init(&page->used.delayed_free_list);
        refcount_init(&page->used.refcount);
    }

    if (alloc_flags & __ALLOC_ZERO) {
        zero_multiple_pages(page_to_virt(page), 1ull << order);
    }

    return page;
}

struct page *
alloc_pages(const enum page_state state,
            const uint64_t alloc_flags,
            const uint8_t order)
{
    if (order >= MAX_ORDER) {
        printk(LOGLEVEL_WARN, "mm: alloc_pages() got order >= MAX_ORDER\n");
        return NULL;
    }

    struct page_zone *const start = page_alloc_flags_to_zone(alloc_flags);
    struct page_zone *zone = start;

    if (__builtin_expect(zone == NULL, 0)) {
        printk(LOGLEVEL_WARN,
               "mm: page_alloc_flags_to_zone() returned null in "
               "alloc_pages()\n");
        return NULL;
    }

    struct page *page = NULL;
    while (zone != NULL) {
        page = alloc_pages_from_zone(zone, order, state);
        if (page != NULL) {
            return setup_alloced_page(page, state, alloc_flags, order);
        }

        zone = zone->fallback_zone;
    }

    return NULL;
}

__optimize(3) static struct page *
alloc_large_page_from_zone(struct page_zone *const zone,
                           const struct largepage_level_info *const info)
{
    const int flag = spin_acquire_with_irq(&zone->lock);
    const uint8_t order = info->order;

    if (zone->total_free < (1ull << order)) {
        spin_release_with_irq(&zone->lock, flag);
        return NULL;
    }

    // zone->min_order should never equal MAX_ORDER if zone->total_free is not 0
    uint8_t alloced_order = max(order, zone->min_order);
    while (true) {
        struct page_freelist *const freelist =
            &zone->freelist_list[alloced_order];
        struct page *const page =
            get_large_from_freelist(zone, freelist, order);

        if (page != NULL) {
            spin_release_with_irq(&zone->lock, flag);
            return page;
        }

        alloced_order++;
        if (alloced_order >= MAX_ORDER) {
            spin_release_with_irq(&zone->lock, flag);
            return NULL;
        }
    }

    spin_release_with_irq(&zone->lock, flag);
    return NULL;
}

struct page *
alloc_large_page(const uint64_t alloc_flags, const pgt_level_t level) {
    struct page_zone *const start = page_alloc_flags_to_zone(alloc_flags);
    struct page_zone *zone = start;

    if (__builtin_expect(zone == NULL, 0)) {
        printk(LOGLEVEL_WARN,
               "mm: alloc_large_page() page_alloc_flags_to_zone() returned "
               "null\n");
        return NULL;
    }

    if (__builtin_expect(!largepage_level_info_list[level].is_supported, 0)) {
        printk(LOGLEVEL_WARN,
               "mm: alloc_large_page() allocating at level %" PRIu8 " is not "
               "supported\n",
               level);
        return NULL;
    }

    struct page *page = NULL;
    const struct largepage_level_info *const info =
        &largepage_level_info_list[level];

    const uint8_t order = info->order;
    if (__builtin_expect(order >= MAX_ORDER, 0)) {
        printk(LOGLEVEL_WARN,
               "mm: alloc_large_page() can't allocate large-page, too large\n");
        return NULL;
    }

    while (zone != NULL) {
        page = alloc_large_page_from_zone(zone, info);
        if (page != NULL) {
            return setup_alloced_page(page,
                                      PAGE_STATE_LARGE_HEAD,
                                      alloc_flags,
                                      order);
        }

        zone = zone->fallback_zone;
    }

    return NULL;
}

__optimize(3) void
early_free_pages_to_zone(struct page *const page,
                         struct page_zone *const zone,
                         const uint8_t order)
{
    page->buddy.order = order;
    zone->total_free += 1ull << order;

    early_add_to_freelist(zone, &zone->freelist_list[order], page);
}

__optimize(3) void
free_pages_to_zone(struct page *const page,
                   struct page_zone *const zone,
                   const uint8_t order)
{
    const int flag = spin_acquire_with_irq(&zone->lock);

    free_pages_to_zone_unlocked(page, zone, order);
    spin_release_with_irq(&zone->lock, flag);
}

void free_large_page_to_zone(struct page *head, struct page_zone *const zone) {
    const int flag = spin_acquire_with_irq(&zone->lock);

    const pgt_level_t level = head->largehead.level;
    const uint64_t page_count = 1ull << largepage_level_info_list[level].order;

    struct mm_section *const memmap = page_to_mm_section(head);
    const struct page *const end = head + page_count;

    for (struct page *page = head; page < end;) {
        if (!ref_down(&page->largehead.page_refcount)) {
            page++;
            continue;
        }

        struct page *iter = page + 1;
        for (; iter != end; iter++) {
            if (!ref_down(&iter->largehead.page_refcount)) {
                break;
            }
        }

        free_range_of_pages(page, zone, (uint64_t)(iter - page));
        page = iter + 1;
    }

    const uint64_t page_index = page_to_pfn(head) - memmap->pfn;
    const struct range set_range = range_create(page_index, page_count);

    bitmap_set_range(&memmap->used_pages_bitmap,
                     set_range,
                     /*value=*/false);

    spin_release_with_irq(&zone->lock, flag);
}

void free_pages(struct page *const page, const uint8_t order) {
    if (order >= MAX_ORDER) {
        printk(LOGLEVEL_WARN, "mm: free_pages() got order >= MAX_ORDER\n");
        return;
    }

    if (page_get_state(page) == PAGE_STATE_LARGE_HEAD) {
        if (order != 0) {
            printk(LOGLEVEL_WARN,
                   "mm: free_pages() got order > 0, which is not supported for "
                   "large pages\n");
            return;
        }

        free_large_page_to_zone(page, page_to_zone(page));
        return;
    }

    free_pages_to_zone(page, page_to_zone(page), order);
}

void free_large_page(struct page *const page) {
    assert(page_get_state(page) == PAGE_STATE_LARGE_HEAD);
    free_large_page_to_zone(page, page_to_zone(page));
}

__optimize(3)
struct page *deref_page(struct page *page, struct pageop *const pageop) {
    const enum page_state state = page_get_state(page);
    assert(__builtin_expect(state != PAGE_STATE_LARGE_HEAD, 1));

    if (ref_down(&page->used.refcount)) {
        list_add(&pageop->delayed_free, &page->used.delayed_free_list);
        return NULL;
    }

    return page;
}