/*
 * kernel/mm/page_alloc.c
 * © suhas pai
 */

#include "dev/printk.h"

#include "mm/page.h"
#include "mm/zone.h"

static void
add_to_freelist(struct page_freelist *const freelist, struct page *const page) {
    set_page_bit(page, PAGE_IN_FREELIST);
    list_add(&freelist->pages, &page->buddy.list);

    freelist->count++;
}

static struct page *get_from_freelist(struct page_freelist *const freelist) {
    if (list_empty(&freelist->pages)) {
        return NULL;
    }

    struct page *page =
        list_head(&freelist->pages, struct page, buddy.list);

    list_delete(&page->buddy.list);
    clear_page_bit(page, PAGE_IN_FREELIST);

    freelist->count--;
    return page;
}


static struct page *
take_off_freelist(struct page_freelist *const freelist,
                  struct page *const page)
{
    if (list_empty(&freelist->pages)) {
        return NULL;
    }

    list_delete(&page->buddy.list);
    clear_page_bit(page, PAGE_IN_FREELIST);

    freelist->count--;
    return page;
}

static inline
struct page *buddy_of(struct page *const page, const uint8_t order) {
    return pfn_to_page(page_to_pfn(page) ^ (1ull << order));
}

static struct page *
alloc_page_from_zone(struct page_zone *const zone, const uint8_t order) {
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

struct page *alloc_pages(const uint8_t order, const uint64_t alloc_flags) {
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
        page = alloc_page_from_zone(zone, order);
        if (page != NULL) {
            goto done;
        }

        zone = zone->fallback_zone;
    }

    return NULL;
done:
    if (alloc_flags & PG_ALLOC_ZERO) {
        memzero(page_to_virt(page), PAGE_SIZE);
    }

    if (alloc_flags & PG_ALLOC_PTE) {
        refcount_init(&page->pte.refcount);
        list_init(&page->pte.delayed_free_list);
    } else if (alloc_flags & PG_ALLOC_SLAB_HEAD) {
        set_page_bit(page, PAGE_SLAB_HEAD);
        list_init(&page->slab.head.slab_list);
    }

    return page;
}

void
free_page_to_zone(struct page *page,
                  struct page_zone *const zone,
                  uint8_t order)
{
    const int flag = spin_acquire_with_irq(&zone->lock);
    while (true) {
		if (order == MAX_ORDER - 1) {
			break;
        }

		struct page *buddy = buddy_of(page, order);
		if (zone != page_to_zone(buddy)) {
			break;
        }

		if (!has_page_bit(buddy, PAGE_IN_FREELIST)) {
			break;
        }

		if (buddy->buddy.order != order) {
			break;
        }

		take_off_freelist(&zone->freelist_list[order], buddy);
		if (page_to_pfn(buddy) < page_to_pfn(page)) {
			swap(page, buddy);
        }

		order++;
	}

	page->buddy.order = order;

	add_to_freelist(&zone->freelist_list[order], page);
	spin_release_with_irq(&zone->lock, flag);
}

void free_pages(struct page *const page, uint8_t order) {
    if (order >= MAX_ORDER) {
        printk(LOGLEVEL_WARN, "free_pages(): got order >= MAX_ORDER");
        return;
    }

    struct page_zone *const zone = page_to_zone(page);
    free_page_to_zone(page, zone, order);
}