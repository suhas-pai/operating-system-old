/*
 * kernel/mm/page_alloc.c
 * © suhas pai
 */

#include <stdatomic.h>

#include "dev/printk.h"
#include "lib/align.h"

#include "page.h"
#include "zone.h"

__optimize(3) static void
add_to_freelist(struct page_zone *const zone,
                struct page_freelist *const freelist,
                struct page *const page)
{
    page_set_state(page, PAGE_STATE_FREE_LIST_HEAD);

    const uint8_t freelist_order = freelist - zone->freelist_list;
    page->freelist_head.order = freelist_order;

    list_init(&page->freelist_head.freelist);
    list_add(&freelist->page_list, &page->freelist_head.freelist);

    const struct page *const end = page + (1ull << freelist_order);

    for (struct page *iter = page + 1; iter != end; iter++) {
        page_set_state(iter, PAGE_STATE_FREE_LIST_TAIL);
        iter->freelist_tail.head = page;
    }

    freelist->count++;
    zone->total_free += 1ull << freelist_order;

    if (zone->min_order > freelist_order) {
        zone->min_order = freelist_order;
    }
}

__optimize(3) static void
early_add_to_freelist(struct page_zone *const zone,
                      struct page_freelist *const freelist,
                      struct page *const page)
{
    const uint8_t freelist_order = freelist - zone->freelist_list;
    page->freelist_head.order = freelist_order;

    list_init(&page->freelist_head.freelist);
    list_add(&freelist->page_list, &page->freelist_head.freelist);

    page->state = PAGE_STATE_FREE_LIST_HEAD;
    freelist->count++;

    const struct page *const end = page + (1ull << freelist_order);
    for (struct page *iter = page + 1; iter != end; iter++) {
        iter->freelist_tail.head = page;
    }

    zone->total_free += 1ull << freelist_order;
    if (zone->min_order > freelist_order) {
        zone->min_order = freelist_order;
    }
}

// Setup pages that just came off the freelist. This setup needs to be as quick
// as possible because this is done under the zone's lock.

__optimize(3) static inline void
setup_pages_off_freelist(struct page *const page,
                         const uint8_t order,
                         const enum page_state state)
{
    switch (state) {
        case PAGE_STATE_SYSTEM_CRUCIAL:
            verify_not_reached();
        case PAGE_STATE_USED: {
            const uint64_t page_count = 1ull << order;
            const struct page *const end = page + page_count;

            for (struct page *iter = page; iter != end; iter++) {
                page_set_state(page, state);
            }

            return;
        }
        case PAGE_STATE_FREE_LIST_HEAD:
        case PAGE_STATE_FREE_LIST_TAIL:
        case PAGE_STATE_LRU_CACHE:
            verify_not_reached();
        case PAGE_STATE_SLAB_HEAD: {
            const uint64_t page_count = 1ull << order;
            const struct page *const end = page + page_count;

            page_set_state(page, PAGE_STATE_SLAB_HEAD);
            for (struct page *iter = page + 1; iter != end; iter++) {
                page_set_state(iter, PAGE_STATE_SLAB_TAIL);
                iter->slab.tail.head = page;
            }

            return;
        }
        case PAGE_STATE_SLAB_TAIL:
            verify_not_reached();
        case PAGE_STATE_TABLE:
            page_set_state(page, state);
            return;
        case PAGE_STATE_LARGE_HEAD: {
            const uint64_t page_count = 1ull << order;
            const struct page *const end = page + page_count;

            page_set_state(page, PAGE_STATE_LARGE_HEAD);
            for (struct page *iter = page + 1; iter != end; iter++) {
                page_set_state(iter, PAGE_STATE_LARGE_TAIL);
                refcount_init(&iter->largetail.refcount);

                iter->largetail.head = page;
            }

            return;
        }
        case PAGE_STATE_LARGE_TAIL:
            verify_not_reached();
    }

    verify_not_reached();
}

__optimize(3) static struct page *
take_off_freelist(struct page_zone *const zone,
                  struct page_freelist *freelist,
                  struct page *const page)
{
    list_delete(&page->freelist_head.freelist);
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
                  struct page_freelist *const freelist)
{
    struct page *const page =
        list_head(&freelist->page_list, struct page, freelist_head.freelist);

    if (__builtin_expect(
            &page->freelist_head.freelist == &freelist->page_list, 0))
    {
        return NULL;
    }

    return take_off_freelist(zone, freelist, page);
}

__optimize(3) uint64_t buddy_of(const uint64_t pfn, const uint8_t order) {
    return pfn ^ (1ull << order);
}

void
free_range_of_pages(struct page *page,
                    uint64_t page_pfn,
                    struct page_zone *const zone,
                    const uint64_t amount)
{
    int8_t order = MAX_ORDER - 1;
    uint64_t avail = amount;

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

        add_to_freelist(zone, &zone->freelist_list[max_free_order], page);

        const uint64_t page_count = 1ull << max_free_order;
        avail -= page_count;

        if (avail == 0) {
            break;
        }

        page += page_count;
        page_pfn += page_count;

        do {
            if (avail >= (1ull << order)) {
                break;
            }

            order--;
            if (order < 0) {
                return;
            }
        } while (true);
    } while (true);
}

__optimize(3) static struct page *
get_large_from_freelist(struct page_zone *const zone,
                        struct page_freelist *const freelist,
                        const uint8_t largepage_order)
{
    const uint8_t freelist_order = freelist - zone->freelist_list;
    struct page *head =
        list_head(&freelist->page_list, struct page, freelist_head.freelist);

    if (&head->freelist_head.freelist == &freelist->page_list) {
        return NULL;
    }

    struct page *page = head;
    uint64_t page_phys = page_to_phys(page);

    if (largepage_order != freelist_order) {
        take_off_freelist(zone, freelist, head);
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

            const uint64_t free_amount = (uint64_t)(page - head);
            free_range_of_pages(head, page_to_pfn(head), zone, free_amount);
        }

        setup_pages_off_freelist(page, largepage_order, PAGE_STATE_LARGE_HEAD);
        struct page *begin = page + (1ull << largepage_order);

        const struct page *const end = head + (1ull << freelist_order);
        const uint64_t count = (uint64_t)(end - begin);

        if (count != 0) {
            free_range_of_pages(begin, page_to_pfn(begin), zone, count);
        }
    } else {
        const uint64_t align = PAGE_SIZE << largepage_order;
        do {
            if (has_align(page_phys, align)) {
                take_off_freelist(zone, freelist, head);
                break;
            }

            head =
                container_of(head->freelist_head.freelist.next,
                             struct page,
                             freelist_head.freelist);

            if (&head->freelist_head.freelist == &freelist->page_list) {
                return NULL;
            }

            page = head;
            page_phys = page_to_phys(page);
        } while (true);

        setup_pages_off_freelist(page, largepage_order, PAGE_STATE_LARGE_HEAD);
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

        page = get_from_freelist(zone, freelist);
        if (page != NULL) {
            break;
        }

        alloced_order++;
        if (alloced_order >= MAX_ORDER) {
            spin_release_with_irq(&zone->lock, flag);
            return NULL;
        }
    }

    struct mm_section *const section = page_to_mm_section(page);

    const uint64_t section_pfn = section->pfn;
    const uint64_t page_pfn = page_to_pfn(page) - section_pfn;

    while (alloced_order > order) {
        alloced_order--;
        struct page *const buddy_page =
            pfn_to_page(section_pfn + buddy_of(page_pfn, alloced_order));

        add_to_freelist(zone, &zone->freelist_list[alloced_order], buddy_page);
    }

    setup_pages_off_freelist(page, order, state);
    spin_release_with_irq(&zone->lock, flag);

    return page;
}

__optimize(3) struct page *
setup_alloced_page(struct page *const page,
                   const enum page_state state,
                   const uint64_t alloc_flags,
                   const uint8_t order,
                   const struct largepage_level_info *const largeinfo)
{
    switch (state) {
        case PAGE_STATE_SYSTEM_CRUCIAL:
            verify_not_reached();
        case PAGE_STATE_USED: {
            const uint64_t page_count = 1ull << order;
            const struct page *const end = page + page_count;

            for (struct page *iter = page; iter != end; iter++) {
                list_init(&page->used.delayed_free_list);
                refcount_init(&page->used.refcount);
            }

            if (alloc_flags & __ALLOC_ZERO) {
                zero_multiple_pages(page_to_virt(page), page_count);
            }

            return page;
        }
        case PAGE_STATE_FREE_LIST_HEAD:
        case PAGE_STATE_FREE_LIST_TAIL:
        case PAGE_STATE_LRU_CACHE:
            verify_not_reached();
        case PAGE_STATE_SLAB_HEAD:
            zero_multiple_pages(page_to_virt(page), 1ull << order);
            list_init(&page->slab.head.slab_list);

            return page;
        case PAGE_STATE_SLAB_TAIL:
            verify_not_reached();
        case PAGE_STATE_TABLE:
            zero_page(page_to_virt(page));
            list_init(&page->table.delayed_free_list);

            page->table.refcount = REFCOUNT_EMPTY();
            return page;
        case PAGE_STATE_LARGE_HEAD:
            refcount_init(&page->largehead.refcount);
            refcount_init(&page->largehead.page_refcount);

            list_init(&page->largehead.delayed_free_list);
            page->largehead.level = largeinfo->level;

            if (alloc_flags & __ALLOC_ZERO) {
                const uint64_t page_count = 1ull << order;
                zero_multiple_pages(page_to_virt(page), page_count);
            }

            return page;
        case PAGE_STATE_LARGE_TAIL:
            verify_not_reached();
    }

    verify_not_reached();
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

    struct page_zone *zone = page_zone_default();
    struct page *page = NULL;

    while (zone != NULL) {
        page = alloc_pages_from_zone(zone, order, state);
        if (page != NULL) {
            return setup_alloced_page(page,
                                      state,
                                      alloc_flags,
                                      order,
                                      /*largeinfo=*/NULL);
        }

        zone = zone->fallback_zone;
    }

    return NULL;
}

struct page *
alloc_pages_in_zone(struct page_zone *zone,
                    const enum page_state state,
                    const uint64_t alloc_flags,
                    const uint8_t order,
                    const bool fallback)
{
    if (order >= MAX_ORDER) {
        printk(LOGLEVEL_WARN, "mm: alloc_pages() got order >= MAX_ORDER\n");
        return NULL;
    }

    struct page *page = alloc_pages_from_zone(zone, order, state);
    if (page != NULL) {
    setup:
        return setup_alloced_page(page,
                                  state,
                                  alloc_flags,
                                  order,
                                  /*largeinfo=*/NULL);
    }

    if (!fallback) {
        return NULL;
    }

    do {
        zone = zone->fallback_zone;
        if (zone == NULL) {
            break;
        }

        page = alloc_pages_from_zone(zone, order, state);
        if (page != NULL) {
            goto setup;
        }

    } while (true);

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
    struct page_zone *zone = page_zone_iterstart();
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
                                      order,
                                      info);
        }

        zone = zone->fallback_zone;
    }

    return NULL;
}

struct page *
alloc_large_page_in_zone(struct page_zone *zone,
                         const uint64_t alloc_flags,
                         const pgt_level_t level,
                         const bool fallback)
{
    if (__builtin_expect(!largepage_level_info_list[level].is_supported, 0)) {
        printk(LOGLEVEL_WARN,
               "mm: alloc_large_page() allocating at level %" PRIu8 " is not "
               "supported\n",
               level);
        return NULL;
    }

    const struct largepage_level_info *const info =
        &largepage_level_info_list[level];

    const uint8_t order = info->order;
    if (__builtin_expect(order >= MAX_ORDER, 0)) {
        printk(LOGLEVEL_WARN,
               "mm: alloc_large_page() can't allocate large-page, too large\n");
        return NULL;
    }

    struct page *page = alloc_large_page_from_zone(zone, info);
    if (page != NULL) {
    setup:
        return setup_alloced_page(page,
                                  PAGE_STATE_LARGE_HEAD,
                                  alloc_flags,
                                  order,
                                  info);
    }

    if (!fallback) {
        return NULL;
    }

    do {
        zone = zone->fallback_zone;
        if (zone == NULL) {
            break;
        }

        page = alloc_large_page_from_zone(zone, info);
        if (page != NULL) {
            goto setup;
        }
    } while (true);

    return NULL;
}

__optimize(3) void
early_free_pages_to_zone(struct page *const page,
                         struct page_zone *const zone,
                         const uint8_t order)
{
    early_add_to_freelist(zone, &zone->freelist_list[order], page);
}

__optimize(3) void
free_pages_to_zone(struct page *page, struct page_zone *zone, uint8_t order) {
    const uint64_t page_section = page->section;
    struct mm_section *const section = &mm_get_usable_list()[page_section];

    const uint64_t section_pfn = section->pfn;
    const uint64_t max_pfn = PAGE_COUNT(section->range.size);

    uint64_t page_pfn = page_to_pfn(page) - section_pfn;
    const int flag = spin_acquire_with_irq(&zone->lock);

    for (; order < MAX_ORDER - 1; order++) {
        const uint64_t buddy_pfn = buddy_of(page_pfn, order);
        if (buddy_pfn > max_pfn) {
            break;
        }

        struct page *const buddy = pfn_to_page(section_pfn + buddy_pfn);
        const enum page_state buddy_state = page_get_state(buddy);

        if (buddy_state != PAGE_STATE_FREE_LIST_HEAD &&
            buddy_state != PAGE_STATE_FREE_LIST_TAIL)
        {
            break;
        }

        if (buddy->freelist_head.order != order) {
            break;
        }

        const uint64_t buddy_phys =
            section->range.front + (buddy_pfn << PAGE_SHIFT);

        if (zone != phys_to_zone(buddy_phys)) {
            break;
        }

        take_off_freelist(zone, &zone->freelist_list[order], buddy);
        if (buddy_pfn < page_pfn) {
            page = buddy;
            page_pfn = buddy_pfn;
        }
    }

    add_to_freelist(zone, &zone->freelist_list[order], page);
    spin_release_with_irq(&zone->lock, flag);
}

void
free_range_of_pages_and_look_around(struct page *const page,
                                    struct page_zone *const zone,
                                    uint64_t amount)
{
    const struct mm_section *const section = page_to_mm_section(page);
    uint64_t page_pfn = page_to_pfn(page) - section->pfn;

    struct page *free_page = page;
    uint64_t free_page_pfn = page_pfn;

    if (__builtin_expect(page_pfn != 0, 1)) {
        const enum page_state prev_state = page_get_state(free_page - 1);
        if (prev_state == PAGE_STATE_FREE_LIST_TAIL) {
            free_page = (free_page - 1)->freelist_tail.head;

            amount += (uint64_t)(page - free_page);
            free_page_pfn -= (uint64_t)(page - free_page);

            const uint8_t order = free_page->freelist_head.order;
            take_off_freelist(zone, &zone->freelist_list[order], free_page);
        } else if (prev_state == PAGE_STATE_FREE_LIST_HEAD) {
            free_page--;
            free_page_pfn--;

            amount += 1;

            const uint8_t order = free_page->freelist_head.order;
            take_off_freelist(zone, &zone->freelist_list[order], free_page);
        }
    }

    struct page *const end_page = free_page + amount;
    if (__builtin_expect((uint64_t)end_page < PAGE_END, 1)) {
        if (page_get_state(end_page) == PAGE_STATE_FREE_LIST_HEAD) {
            const uint8_t order = end_page->freelist_head.order;
            take_off_freelist(zone, &zone->freelist_list[order], end_page);

            amount += 1ull << order;
        }
    }

    return free_range_of_pages(free_page, free_page_pfn, zone, amount);
}

void free_large_page_to_zone(struct page *head, struct page_zone *const zone) {
    const int flag = spin_acquire_with_irq(&zone->lock);

    const pgt_level_t level = head->largehead.level;
    const uint64_t page_count = 1ull << largepage_level_info_list[level].order;

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

        const uint64_t free_amount = (uint64_t)(iter - page);
        free_range_of_pages_and_look_around(page, zone, free_amount);

        page = iter + 1;
    }

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

struct page *
deref_large_page(struct page *const page,
                 struct pageop *const pageop,
                 const pgt_level_t level)
{
    if (page_get_state(page) == PAGE_STATE_LARGE_HEAD) {
        if (ref_down(&page->largehead.refcount)) {
            list_add(&pageop->delayed_free, &page->used.delayed_free_list);
            return NULL;
        }

        return page;
    }

    const struct page *const end =
        page + (1ull << largepage_level_info_list[level].order);

    for (struct page *iter = page; iter != end; iter++) {
        deref_page(iter, pageop);
    }

    return NULL;
}

struct page *alloc_table() {
    return alloc_page(PAGE_STATE_TABLE, __ALLOC_ZERO);
}