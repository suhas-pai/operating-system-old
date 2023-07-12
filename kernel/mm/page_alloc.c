/*
 * kernel/mm/page_alloc.c
 * © suhas pai
 */

#include "dev/printk.h"

#include "mm/page.h"
#include "mm/zone.h"

static void
add_to_freelist(struct page_freelist *const freelist, struct page *const page) {
    page_set_bit(page, PAGE_IN_FREE_LIST);
    list_add(&freelist->pages, &page->buddy.freelist);

    freelist->count++;
}

static struct page *
take_off_freelist(struct page_freelist *const freelist, struct page *const page)
{
    list_delete(&page->buddy.freelist);
    page_clear_bit(page, PAGE_IN_FREE_LIST);

    freelist->count--;
    return page;
}

static struct page *get_from_freelist(struct page_freelist *const freelist) {
    if (list_empty(&freelist->pages)) {
        return NULL;
    }

    struct page *const page =
        list_head(&freelist->pages, struct page, buddy.freelist);

    return take_off_freelist(freelist, page);
}

static inline
struct page *buddy_of(struct page *const page, const uint8_t order) {
    return pfn_to_page(page_to_pfn(page) ^ (1ull << order));
}

static struct page *
alloc_pages_from_zone(struct page_zone *const zone, const uint8_t order) {
    uint8_t alloced_order = 0;

    struct page *page = NULL;
    const int flag = spin_acquire_with_irq(&zone->lock);

    while (true) {
        struct page_freelist *const freelist =
            &zone->freelist_list[alloced_order];

        struct page *const result = get_from_freelist(freelist);
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

        struct page *const buddy_page = buddy_of(page, order);
        buddy_page->buddy.order = order;

        add_to_freelist(&zone->freelist_list[alloced_order], buddy_page);
    }

    spin_release_with_irq(&zone->lock, flag);
    return NULL;
}

struct page *alloc_pages(const uint64_t alloc_flags, const uint8_t order) {
    if (order >= MAX_ORDER) {
        printk(LOGLEVEL_WARN, "alloc_pages(): got order >= MAX_ORDER");
        return NULL;
    }

    struct page_zone *zone = page_alloc_flags_to_zone(alloc_flags);
    if (zone == NULL) {
        printk(LOGLEVEL_WARN,
               "alloc_pages(): page_alloc_flags_to_zone() returned null");
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
    if (alloc_flags & __ALLOC_ZERO) {
        memzero(page_to_virt(page), PAGE_SIZE);
    }

    if (alloc_flags & __ALLOC_TABLE) {
        refcount_init(&page->table.refcount);
        list_init(&page->table.delayed_free_list);
    } else if (alloc_flags & __ALLOC_SLAB_HEAD) {
        page_set_bit(page, PAGE_IS_SLAB_HEAD);
        list_init(&page->slab.head.slab_list);
    }

    return page;
}

void
free_pages_to_zone(struct page *page,
                   struct page_zone *const zone,
                   uint8_t order)
{
    const int flag = spin_acquire_with_irq(&zone->lock);
    for (; order != MAX_ORDER - 1; order++) {
        struct page *buddy = buddy_of(page, order);
        if (zone != page_to_zone(buddy)) {
            break;
        }

        if (page_has_bit(buddy, PAGE_NOT_USABLE)) {
            continue;
        }

        if (!page_has_bit(buddy, PAGE_IN_FREE_LIST)) {
            break;
        }

        if (buddy->buddy.order != order) {
            break;
        }

        take_off_freelist(&zone->freelist_list[order], buddy);
        if (page_to_pfn(buddy) < page_to_pfn(page)) {
            swap(page, buddy);
        }
    }

    page->buddy.order = order;

    add_to_freelist(&zone->freelist_list[order], page);
    spin_release_with_irq(&zone->lock, flag);
}

void free_pages(struct page *const page, const uint8_t order) {
    if (order >= MAX_ORDER) {
        printk(LOGLEVEL_WARN, "free_pages(): got order >= MAX_ORDER");
        return;
    }

    struct page_zone *const zone = page_to_zone(page);
    free_pages_to_zone(page, zone, order);
}