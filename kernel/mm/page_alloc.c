/*
 * kernel/mm/page_alloc.c
 * © suhas pai
 */

#include <stdatomic.h>

#include "dev/printk.h"
#include "lib/align.h"
#include "mm/zone.h"

__optimize(3) static void
add_to_freelist(struct page_zone *const zone,
                struct page_freelist *const freelist,
                struct page *const page)
{
    page_set_bit(page, PAGE_IN_FREE_LIST);

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
    struct page *page =
        list_head(&freelist->page_list, struct page, buddy.freelist);

    return take_off_freelist(zone, freelist, page);
}

__optimize(3) void
setup_large_page(struct page *const begin,
                 const struct largepage_level_info *const info)
{
    const struct page *const end = begin + (1ull << info->order);
    refcount_init(&begin->large.largehead.refcount);

    begin->large.head = begin;
    begin->large.largehead.level = info->level;

    for (struct page *page = begin; page != end; page++) {
        page_set_bit(page, PAGE_IN_LARGE_PAGE);
        refcount_init(&page->large.page_refcount);

        page->large.head = begin;
    }
}

void
early_free_pages_to_zone(struct page *page,
                         struct page_zone *zone,
                         uint8_t order);

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
            const uint64_t buddy_pfn = page_pfn ^ (1ull << iorder);
            if (buddy_pfn < page_pfn) {
                max_free_order = (uint8_t)iorder;
            }
        }

        early_free_pages_to_zone(page, zone, max_free_order);

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
        if (!has_align(page_phys, PAGE_SIZE << largepage_order)) {
            uint64_t new_phys = 0;
            if (!align_up(page_phys, PAGE_SIZE << largepage_order, &new_phys)) {
                printk(LOGLEVEL_INFO,
                       "alloc_large_page(): failed to align page's physical "
                       "address to boundary at order %" PRIu8 "\n",
                       largepage_order);
                return NULL;
            }

            take_off_freelist(zone, freelist, head);

            page += PAGE_COUNT(new_phys - page_phys);
            page_phys = new_phys;

            free_range_of_pages(head, zone, (uint64_t)(page - head));
        } else {
            take_off_freelist(zone, freelist, head);
        }

        struct page *begin = page + (1ull << largepage_order);
        const struct page *const end = head + (1ull << freelist_order);

        free_range_of_pages(begin, zone, (uint64_t)(end - begin));
    } else {
        do {
            if (has_align(page_phys, PAGE_SIZE << largepage_order)) {
                take_off_freelist(zone, freelist, head);
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

__optimize(3)
static inline uint64_t buddy_of(const uint64_t pfn, const uint8_t order) {
    return pfn ^ (1ull << order);
}

__optimize(3) static struct page *
alloc_pages_from_zone(struct page_zone *const zone, const uint8_t order) {
    struct page *page = NULL;
    const int flag = spin_acquire_with_irq(&zone->lock);

    if (__builtin_expect(zone->total_free < (1ull << order), 0)) {
        spin_release_with_irq(&zone->lock, flag);
        return NULL;
    }

    uint8_t alloced_order = max(order, zone->min_order);
    if (__builtin_expect(alloced_order == MAX_ORDER, 0)) {
        spin_release_with_irq(&zone->lock, flag);
        return NULL;
    }

    while (true) {
        struct page_freelist *const freelist =
            zone->freelist_list + alloced_order;

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

    if (alloc_flags & __ALLOC_TABLE) {
        zero_multiple_pages(page_to_virt(page), 1ull << order);
        list_init(&page->table.delayed_free_list);

        page->table.refcount = REFCOUNT_EMPTY();
        return page;
    }

    if (alloc_flags & __ALLOC_SLAB_HEAD) {
        page_set_bit(page, PAGE_IS_SLAB_HEAD);

        zero_multiple_pages(page_to_virt(page), 1ull << order);
        list_init(&page->slab.head.slab_list);

        return page;
    }

    if (alloc_flags & __ALLOC_ZERO) {
        zero_multiple_pages(page_to_virt(page), 1ull << order);
    }

    return page;
}

struct page *alloc_pages(const uint64_t alloc_flags, const uint8_t order) {
    if (order >= MAX_ORDER) {
        printk(LOGLEVEL_WARN, "alloc_pages(): got order >= MAX_ORDER\n");
        return NULL;
    }

    struct page_zone *const start = page_alloc_flags_to_zone(alloc_flags);
    struct page_zone *zone = start;

    if (__builtin_expect(zone == NULL, 0)) {
        printk(LOGLEVEL_WARN,
               "alloc_pages(): page_alloc_flags_to_zone() returned null\n");
        return NULL;
    }

    struct page *page = NULL;
    while (zone != NULL) {
        page = alloc_pages_from_zone(zone, order);
        if (page != NULL) {
            return setup_alloced_page(page, alloc_flags, order);
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

    uint8_t alloced_order = max(order, zone->min_order);
    if (alloced_order == MAX_ORDER) {
        spin_release_with_irq(&zone->lock, flag);
        return NULL;
    }

    while (true) {
        struct page_freelist *const freelist =
            zone->freelist_list + alloced_order;

        struct page *const page =
            get_large_from_freelist(zone, freelist, order);

        if (page != NULL) {
            spin_release_with_irq(&zone->lock, flag);
            setup_large_page(page, info);

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
               "alloc_large_page(): page_alloc_flags_to_zone() returned "
               "null\n");
        return NULL;
    }

    if (__builtin_expect(!largepage_level_info_list[level].is_supported, 0)) {
        printk(LOGLEVEL_WARN,
               "alloc_large_page(): large-page at level %" PRIu8 " is not "
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
               "alloc_large_page(): can't allocate large-page, too large\n");
        return NULL;
    }

    while (zone != NULL) {
        page = alloc_large_page_from_zone(zone, info);
        if (page != NULL) {
            return setup_alloced_page(page, alloc_flags, order);
        }

        zone = zone->fallback_zone;
    }

    return NULL;
}

__optimize(3) void
early_free_pages_to_zone(struct page *page,
                         struct page_zone *const zone,
                         uint8_t order)
{
    page->buddy.order = order;
    zone->total_free += 1ull << order;

    early_add_to_freelist(zone, &zone->freelist_list[order], page);
}

__optimize(3) void
free_pages_to_zone(struct page *page,
                   struct page_zone *const zone,
                   uint8_t order)
{
    list_init(&page->buddy.freelist);
    const int flag = spin_acquire_with_irq(&zone->lock);

    uint64_t page_pfn = page_to_pfn(page);
    uint64_t page_section = page_get_section(page);

    for (; order < MAX_ORDER - 1; order++) {
        const uint64_t buddy_pfn = buddy_of(page_pfn, order);
        struct page *buddy = pfn_to_page(buddy_pfn);

        // Because both the not-usable flag and the section-number are constant,
        // try speeding up this function by atomically loading flags once rather
        // than twice.

        const uint32_t buddy_flags = page_get_flags(buddy);
        if (buddy_flags & PAGE_NOT_USABLE) {
            break;
        }

        const page_section_t buddy_section =
            (buddy_flags >> SECTION_SHIFT) & SECTION_SHIFT;

        if (page_section != buddy_section) {
            break;
        }

        if (zone != page_to_zone(buddy)) {
            break;
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

__optimize(3) void
free_large_page_to_zone(struct page *page, struct page_zone *const zone) {
    struct page *const head = page->large.head;
    const pgt_level_t level = head->large.largehead.level;

    list_init(&page->buddy.freelist);

    const int flag = spin_acquire_with_irq(&zone->lock);
    uint64_t amount = 0;

    if (page != head) {
        amount = (uint64_t)(page - head);
    } else {
        amount = 1ull << largepage_level_info_list[level].order;
    }

    free_range_of_pages(page, zone, amount);

    struct mm_section *const memmap = page_to_mm_section(page);
    const uint64_t page_index = page_to_pfn(page) - memmap->pfn;

    if (amount != 1) {
        const struct range set_range = range_create(page_index, amount);
        bitmap_set_range(&memmap->used_pages_bitmap,
                         set_range,
                         /*value=*/false);
    } else {
        bitmap_set(&memmap->used_pages_bitmap, page_index, /*value=*/false);
    }

    spin_release_with_irq(&zone->lock, flag);
}

void free_large_page(struct page *const page) {
    assert(page_has_bit(page, PAGE_IN_LARGE_PAGE));
    free_large_page_to_zone(page, page_to_zone(page));
}