/*
 * kernel/mm/page_alloc.c
 * © suhas pai
 */

#include <stdatomic.h>

#include "dev/printk.h"
#include "lib/align.h"

#include "page.h"
#include "zone.h"

// Caller is required to set section->min_order
__optimize(3) static void
add_to_freelist_order(struct page_section *const section,
                      const uint8_t freelist_order,
                      struct page *const page)
{
    page_set_state(page, PAGE_STATE_FREE_LIST_HEAD);
    struct page_freelist *const freelist =
        &section->freelist_list[freelist_order];

    page->freelist_head.order = freelist_order;

    list_init(&page->freelist_head.freelist);
    list_add(&freelist->page_list, &page->freelist_head.freelist);

    const struct page *const end = page + (1ull << freelist_order);
    for (struct page *iter = page + 1; iter != end; iter++) {
        page_set_state(iter, PAGE_STATE_FREE_LIST_TAIL);
        iter->freelist_tail.head = page;
    }

    atomic_fetch_add(&section->zone->total_free, 1ull << freelist_order);

    section->total_free += 1ull << freelist_order;
    freelist->count++;
}

// Add pages from the tail pages of a higher order into a lower order
__optimize(3) static void
add_to_freelist_order_from_higher(struct page_section *const section,
                                  const uint8_t freelist_order,
                                  struct page *const page)
{
    page_set_state(page, PAGE_STATE_FREE_LIST_HEAD);
    struct page_freelist *const freelist =
        &section->freelist_list[freelist_order];

    page->freelist_head.order = freelist_order;

    list_init(&page->freelist_head.freelist);
    list_add(&freelist->page_list, &page->freelist_head.freelist);

    const struct page *const end = page + (1ull << freelist_order);
    for (struct page *iter = page + 1; iter != end; iter++) {
        iter->freelist_tail.head = page;
    }

    freelist->count++;
}

__optimize(3) static void
early_add_to_freelist_order(struct page_section *const section,
                            const uint8_t freelist_order,
                            struct page *const page)
{
    struct page_freelist *const freelist =
        &section->freelist_list[freelist_order];

    page->state = PAGE_STATE_FREE_LIST_HEAD;
    page->freelist_head.order = freelist_order;

    list_init(&page->freelist_head.freelist);
    list_add(&freelist->page_list, &page->freelist_head.freelist);

    const struct page *const end = page + (1ull << freelist_order);
    for (struct page *iter = page + 1; iter != end; iter++) {
        iter->freelist_tail.head = page;
    }

    section->zone->total_free += 1ull << freelist_order;
    section->total_free += 1ull << freelist_order;

    freelist->count++;
}

__optimize(3) struct page *
take_off_freelist_to_add_later(struct page_section *const section,
                               const uint8_t freelist_order,
                               struct page *const page)
{
    list_delete(&page->freelist_head.freelist);
    struct page_freelist *const freelist =
        &section->freelist_list[freelist_order];

    freelist->count--;
    if (section->total_free == 0) {
        section->min_order = 0;
        section->max_order = 0;
    }

    if (freelist->count == 0) {
        if (section->min_order == freelist_order) {
            const struct page_freelist *iter = freelist;
            const struct page_freelist *const end =
                carr_end(section->freelist_list);

            for (; iter != end; iter++) {
                if (iter->count != 0) {
                    break;
                }
            }

            section->min_order = iter - section->freelist_list;
        }

        if (section->max_order == freelist_order) {
            const struct page_freelist *const end = section->freelist_list - 1;
            const struct page_freelist *iter =
                (section->freelist_list + freelist_order) - 1;

            for (; iter != end; iter--) {
                if (iter->count != 0) {
                    break;
                }
            }

            section->max_order = (iter - section->freelist_list) + 1;
        }
    }

    return page;
}

__optimize(3) static struct page *
take_off_freelist_order(struct page_section *const section,
                        const uint8_t freelist_order,
                        struct page *const page,
                        const uint8_t remove_page_order)
{
    atomic_fetch_sub(&section->zone->total_free, 1ull << remove_page_order);
    section->total_free -= 1ull << remove_page_order;

    return take_off_freelist_to_add_later(section, freelist_order, page);
}

__optimize(3) static struct page *
get_from_freelist_order(struct page_section *const section,
                        const uint8_t order,
                        const uint8_t page_remove_order)
{
    struct page_freelist *const freelist = &section->freelist_list[order];
    if (freelist->count == 0) {
        return NULL;
    }

    struct page *const page =
        list_head(&freelist->page_list, struct page, freelist_head.freelist);

    return take_off_freelist_order(section, order, page, page_remove_order);
}

void
free_range_of_pages(struct page *page,
                    struct page_section *const section,
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
        add_to_freelist_order(section, (uint8_t)order, page);

        const uint64_t page_count = 1ull << order;
        avail -= page_count;

        if (avail == 0) {
            if (section->min_order > (uint8_t)order) {
                section->min_order = (uint8_t)order;
            }

            if (section->max_order <= (uint8_t)order) {
                section->max_order = (uint8_t)order + 1;
            }

            break;
        }

        page += page_count;
        do {
            if (avail >= (1ull << order)) {
                break;
            }

            order--;
        } while (true);
    } while (true);
}

// Setup pages that just came off the freelist. This setup needs to be as quick
// as possible because this is done under the section's lock.

__optimize(3) static inline void
setup_pages_off_freelist(struct page *const page,
                         const uint8_t order,
                         const enum page_state state)
{
    switch (state) {
        case PAGE_STATE_SYSTEM_CRUCIAL:
            verify_not_reached();
        case PAGE_STATE_USED: {
            const struct page *const end = page + (1ull << order);
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
            page_set_state(page, PAGE_STATE_SLAB_HEAD);

            const struct page *const end = page + (1ull << order);
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
            page_set_state(page, PAGE_STATE_LARGE_HEAD);

            const struct page *const end = page + (1ull << order);
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
get_large_from_freelist_order(struct page_section *const section,
                              const uint8_t freelist_order,
                              const uint8_t largepage_order)
{
    struct page_freelist *const freelist =
        &section->freelist_list[freelist_order];

    if (freelist->count == 0) {
        return NULL;
    }

    struct page *head =
        list_head(&freelist->page_list, struct page, freelist_head.freelist);

    struct page *page = head;
    uint64_t page_phys = page_to_phys(page);

    const uint64_t align = PAGE_SIZE << largepage_order;
    if (largepage_order != freelist_order) {
        take_off_freelist_order(section, freelist_order, head, freelist_order);
        if (!has_align(page_phys, align)) {
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
            free_range_of_pages(head, section, free_amount);
        }

        setup_pages_off_freelist(page, largepage_order, PAGE_STATE_LARGE_HEAD);
        struct page *begin = page + (1ull << largepage_order);

        const struct page *const end = head + (1ull << freelist_order);
        const uint64_t count = (uint64_t)(end - begin);

        if (count != 0) {
            free_range_of_pages(begin, section, count);
        }
    } else {
        do {
            if (has_align(page_phys, align)) {
                take_off_freelist_order(section,
                                        freelist_order,
                                        head,
                                        freelist_order);
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
try_alloc_pages_from_zone(struct page_zone *const zone,
                          const uint8_t order,
                          const enum page_state state)
{
    struct page *page = NULL;
    if (__builtin_expect(atomic_load(&zone->total_free) < (1ull << order), 0)) {
        return NULL;
    }

    struct page_section *iter = NULL;
    uint8_t alloced_order = 0;
    int flag = 0;

    list_foreach(iter, &zone->section_list, zone_list) {
        if (!spin_try_acquire_with_irq(&iter->lock, &flag)) {
            continue;
        }

        alloced_order = max(order, iter->min_order);
        const uint8_t max_order = iter->max_order;

        for (; alloced_order < max_order; alloced_order++) {
            page = get_from_freelist_order(iter, alloced_order, order);
            if (page != NULL) {
                goto done;
            }
        }

        spin_release_with_irq(&iter->lock, flag);
    }

    return NULL;

done:
    while (alloced_order > order) {
        alloced_order--;

        struct page *const buddy_page = page + (1ull << alloced_order);
        add_to_freelist_order_from_higher(iter, alloced_order, buddy_page);
    }

    if (iter->min_order > order) {
        iter->min_order = order;
    }

    if (iter->max_order <= order) {
        iter->max_order = order + 1;
    }

    spin_release_with_irq(&iter->lock, flag);
    setup_pages_off_freelist(page, order, state);

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
                zero_multiple_pages(page_to_virt(page), 1ull << order);
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
        page = try_alloc_pages_from_zone(zone, order, state);
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
alloc_pages_from_zone(struct page_zone *zone,
                      const enum page_state state,
                      const uint64_t alloc_flags,
                      const uint8_t order,
                      const bool fallback)
{
    if (order >= MAX_ORDER) {
        printk(LOGLEVEL_WARN, "mm: alloc_pages() got order >= MAX_ORDER\n");
        return NULL;
    }

    struct page *page = try_alloc_pages_from_zone(zone, order, state);
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

        page = try_alloc_pages_from_zone(zone, order, state);
        if (page != NULL) {
            goto setup;
        }

    } while (true);

    return NULL;
}

__optimize(3) static struct page *
try_alloc_large_page_from_zone(struct page_zone *const zone,
                               const struct largepage_level_info *const info)
{
    const uint8_t order = info->order;
    if (__builtin_expect(atomic_load(&zone->total_free) < (1ull << order), 0)) {
        return NULL;
    }

    struct page_section *iter = NULL;
    list_foreach(iter, &zone->section_list, zone_list) {
        int flag = 0;
        if (!spin_try_acquire_with_irq(&iter->lock, &flag)) {
            continue;
        }

        uint8_t alloced_order = max(order, iter->min_order);
        const uint8_t max_order = iter->max_order;

        for (; alloced_order < max_order; alloced_order++) {
            struct page *const page =
                get_large_from_freelist_order(iter,
                                              alloced_order,
                                              /*largepage_order=*/order);

            if (page != NULL) {
                spin_release_with_irq(&iter->lock, flag);
                return page;
            }
        }

        spin_release_with_irq(&iter->lock, flag);
    }

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
        page = try_alloc_large_page_from_zone(zone, info);
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
alloc_large_page_from_zone(struct page_zone *zone,
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

    struct page *page = try_alloc_large_page_from_zone(zone, info);
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

        page = try_alloc_large_page_from_zone(zone, info);
        if (page != NULL) {
            goto setup;
        }
    } while (true);

    return NULL;
}

__optimize(3) void
early_free_pages_from_section(struct page *const page,
                              struct page_section *const section,
                              const uint8_t order)
{
    early_add_to_freelist_order(section, order, page);
}

__optimize(3)
void free_amount_of_pages(struct page *const page, uint64_t amount) {
    struct page_section *const section = page_to_section(page);
    uint64_t page_pfn = page_to_pfn(page) - section->pfn;

    struct page *free_page = page;
    if (__builtin_expect(page_pfn != 0, 1)) {
        const enum page_state prev_state = page_get_state(free_page - 1);
        if (prev_state == PAGE_STATE_FREE_LIST_TAIL) {
            free_page = (free_page - 1)->freelist_tail.head;
            amount += (uint64_t)(page - free_page);

            const uint8_t order = free_page->freelist_head.order;
            take_off_freelist_to_add_later(section, order, free_page);
        } else if (prev_state == PAGE_STATE_FREE_LIST_HEAD) {
            free_page--;
            amount += 1;

            const uint8_t order = free_page->freelist_head.order;
            take_off_freelist_to_add_later(section, order, free_page);
        }
    }

    struct page *const end_page = free_page + amount;
    if (__builtin_expect((uint64_t)end_page < PAGE_END, 1)) {
        if (page_get_state(end_page) == PAGE_STATE_FREE_LIST_HEAD) {
            const uint8_t order = end_page->freelist_head.order;
            take_off_freelist_to_add_later(section, order, end_page);

            amount += 1ull << order;
        }
    }

    return free_range_of_pages(free_page, section, amount);
}

void free_large_page(struct page *head) {
    assert(page_get_state(head) == PAGE_STATE_LARGE_HEAD);

    struct page_section *const section = page_to_section(head);
    const int flag = spin_acquire_with_irq(&section->lock);

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

        free_amount_of_pages(page, (uint64_t)(iter - page));
        page = iter + 1;
    }

    spin_release_with_irq(&section->lock, flag);
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

        free_large_page(page);
        return;
    }

    free_amount_of_pages(page, order);
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