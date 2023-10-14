/*
 * kernel/mm/early.c
 * © suhas pai
 */

#include "dev/printk.h"
#include "lib/align.h"

#include "alloc.h"
#include "boot.h"
#include "early.h"

#include "kmalloc.h"
#include "memmap.h"
#include "pagemap.h"
#include "walker.h"

struct freepages_info {
    struct list list;
    struct list asc_list;

    // Number of available pages in this freepages_info struct.
    uint64_t avail_page_count;
    // Count of contiguous pages, starting from this one
    uint64_t total_page_count;
} __page_aligned;

_Static_assert(sizeof(struct freepages_info) <= PAGE_SIZE,
               "freepages_info struct must be small enough to store on a "
               "single page");

// Use two lists to store info on areas of usuable pages;
//  One that stores areas in ascending address order,
//  One that stores areas in ascending order of the number of free pages.

// The address ascending order is needed later in post-arch setup to mark all
// non-usable pages, while the ascending free-page list is used so smaller areas
// are emptied out while larger areas are only used when absolutely necessary.

static struct list g_freepage_list = LIST_INIT(g_freepage_list);
static struct list g_asc_freelist = LIST_INIT(g_asc_freelist);

static uint64_t freepages_list_count = 0;
static uint64_t total_free_pages = 0;
static uint64_t total_free_pages_remaining = 0;

static void add_to_asc_list(struct freepages_info *const info) {
    struct freepages_info *asc_iter = NULL;
    struct freepages_info *asc_prev =
        container_of(&g_asc_freelist, struct freepages_info, asc_list);

    list_foreach(asc_iter, &g_asc_freelist, asc_list) {
        if (info->avail_page_count < asc_iter->avail_page_count) {
            break;
        }

        asc_prev = asc_iter;
    }

    list_add(&asc_prev->asc_list, &info->asc_list);
}

static void claim_pages(const struct mm_memmap *const memmap) {
    struct freepages_info *const info = phys_to_virt(memmap->range.front);

    list_init(&info->list);
    list_init(&info->asc_list);

    const uint64_t page_count = PAGE_COUNT(memmap->range.size);

    total_free_pages += page_count;
    total_free_pages_remaining += page_count;

    info->avail_page_count = page_count;
    info->total_page_count = page_count;

    freepages_list_count++;
    if (freepages_list_count != 1) {
        struct freepages_info *prev = NULL;
        if (prev > info) {
            prev = container_of(&g_freepage_list, struct freepages_info, list);
            struct freepages_info *iter = NULL;

            list_foreach(iter, &g_freepage_list, list) {
                if (info < iter) {
                    break;
                }

                prev = iter;
            }

            if (&prev->list != &g_freepage_list) {
                // Use avail_page_count so we don't merge two memmaps that seem
                // to connect but actually have a block of allocated pages
                // separating them.

                struct freepages_info *next = list_next(prev, list);
                const uint64_t info_size = info->avail_page_count * PAGE_SIZE;

                if ((void *)info + info_size == (void *)next) {
                    info->avail_page_count += page_count;
                    info->total_page_count += page_count;

                    list_delete(&next->list);
                }
            }
        } else {
            prev = list_tail(&g_freepage_list, struct freepages_info, list);
        }

        if (&prev->list != &g_freepage_list) {
            // Use avail_page_count so we don't merge two memmaps that seem to
            // connect but actually have a block of allocated pages separating
            // them.

            const uint64_t back_size = prev->avail_page_count * PAGE_SIZE;
            if ((void *)prev + back_size == (void *)info) {
                prev->avail_page_count += page_count;
                prev->total_page_count += page_count;

                return;
            }
        }

        list_add(&prev->list, &info->list);
    } else {
        list_add(&g_freepage_list, &info->list);
    }

    add_to_asc_list(info);
}

uint64_t early_alloc_page() {
    if (__builtin_expect(list_empty(&g_asc_freelist), 0)) {
        printk(LOGLEVEL_ERROR, "mm: ran out of free-pages\n");
        return INVALID_PHYS;
    }

    struct freepages_info *const info =
        list_head(&g_asc_freelist, struct freepages_info, asc_list);

    // Take the last page out of the list, because first page stores the
    // freepage_info struct.

    info->avail_page_count -= 1;
    if (info->avail_page_count == 0) {
        list_delete(&info->asc_list);
        list_delete(&info->list);
    }

    const uint64_t free_page =
        virt_to_phys(info) + (info->avail_page_count * PAGE_SIZE);

    zero_page(phys_to_virt(free_page));
    total_free_pages_remaining--;

    return free_page;
}

uint64_t early_alloc_multiple_pages(const uint64_t alloc_amount) {
    if (__builtin_expect(list_empty(&g_asc_freelist), 0)) {
        printk(LOGLEVEL_ERROR, "mm: ran out of free-pages\n");
        return INVALID_PHYS;
    }

    struct freepages_info *info = NULL;
    list_foreach(info, &g_asc_freelist, asc_list) {
        if (info->avail_page_count >= alloc_amount) {
            break;
        }
    }

    if (&info->list == &g_asc_freelist) {
        return INVALID_PHYS;
    }

    // Take the last several pages out of the list, because first page stores
    // the freepage_info struct.

    info->avail_page_count -= alloc_amount;
    if (info->avail_page_count == 0) {
        list_delete(&info->list);
        list_delete(&info->asc_list);
    }

    const struct freepages_info *const prev =
        list_prev_safe(info, asc_list, &g_asc_freelist);

    if (prev != NULL && prev->avail_page_count > info->avail_page_count) {
        list_remove(&info->asc_list);
        add_to_asc_list(info);
    }

    const uint64_t free_page =
        virt_to_phys(info) + (info->avail_page_count * PAGE_SIZE);

    zero_multiple_pages(phys_to_virt(free_page), alloc_amount);
    total_free_pages_remaining -= alloc_amount;

    return free_page;
}

uint64_t early_alloc_large_page(const uint32_t alloc_amount) {
    if (__builtin_expect(list_empty(&g_asc_freelist), 0)) {
        printk(LOGLEVEL_ERROR, "mm: ran out of free-pages\n");
        return INVALID_PHYS;
    }

    struct freepages_info *info = NULL;

    uint64_t free_page = INVALID_PHYS;
    bool is_in_middle = false;

    list_foreach(info, &g_asc_freelist, asc_list) {
        const uint64_t avail_page_count = info->avail_page_count;
        if (avail_page_count < alloc_amount) {
            continue;
        }

        uint64_t phys = virt_to_phys(info);
        uint64_t last_phys =
            phys + ((avail_page_count - alloc_amount) * PAGE_SIZE);

        const uint64_t boundary = PAGE_SIZE * alloc_amount;
        if (!has_align(last_phys, boundary)) {
            last_phys = align_down(last_phys, boundary);
            if (last_phys < phys) {
                continue;
            }

            is_in_middle = true;
        }

        free_page = last_phys;
        break;
    }

    if (free_page == INVALID_PHYS) {
        return INVALID_PHYS;
    }

    if (is_in_middle) {
        struct freepages_info *prev = info;
        const uint64_t old_info_count = info->avail_page_count;

        info->avail_page_count = PAGE_COUNT(free_page - virt_to_phys(info));
        info->total_page_count =
            info->avail_page_count + PAGE_COUNT(alloc_amount);

        if (info->avail_page_count != 0) {
            struct freepages_info *const info_prev = list_prev(info, asc_list);
            if (&info->list != &g_asc_freelist &&
                info_prev->avail_page_count > info->avail_page_count)
            {
                list_remove(&info->asc_list);
                add_to_asc_list(info);
            }
        } else {
            prev = list_prev(info, list);

            list_delete(&info->asc_list);
            list_delete(&info->list);
        }

        const uint64_t new_info_phys = free_page + (PAGE_SIZE * alloc_amount);
        const uint64_t new_info_count =
            old_info_count - info->avail_page_count - alloc_amount;

        if (new_info_count != 0) {
            struct freepages_info *const new_info =
                (struct freepages_info *)phys_to_virt(new_info_phys);

            new_info->avail_page_count = new_info_count;
            new_info->total_page_count = new_info_count;

            list_init(&new_info->list);
            list_init(&new_info->asc_list);

            list_add(&prev->list, &new_info->list);
            add_to_asc_list(new_info);
        }
    } else {
        const struct freepages_info *const prev =
            list_prev_safe(info, asc_list, &g_asc_freelist);

        info->avail_page_count -= alloc_amount;
        if (info->avail_page_count == 0) {
            list_delete(&info->list);
            list_delete(&info->asc_list);
        } else if (prev != NULL) {
            if (prev->avail_page_count > info->avail_page_count) {
                list_remove(&info->asc_list);
                add_to_asc_list(info);
            }
        }
    }

    zero_multiple_pages(phys_to_virt(free_page), alloc_amount);
    total_free_pages_remaining -= alloc_amount;

    return free_page;
}

__hidden uint64_t KERNEL_BASE = 0;
__hidden uint64_t structpage_page_count = 0;

void mm_early_init() {
    printk(LOGLEVEL_INFO, "mm: hhdm at %p\n", (void *)HHDM_OFFSET);
    printk(LOGLEVEL_INFO, "mm: kernel at %p\n", (void *)KERNEL_BASE);

    const uint64_t memmap_count = mm_get_memmap_count();
    for (uint64_t index = 0; index != memmap_count; index++) {
        const struct mm_memmap *const memmap = &mm_get_memmap_list()[index];
        const char *type_desc = "<unknown>";

        switch (memmap->kind) {
            case MM_MEMMAP_KIND_NONE:
                verify_not_reached();
            case MM_MEMMAP_KIND_USABLE:
                claim_pages(memmap);

                type_desc = "usable";
                structpage_page_count += PAGE_COUNT(memmap->range.size);

                break;
            case MM_MEMMAP_KIND_RESERVED:
                type_desc = "reserved";
                break;
            case MM_MEMMAP_KIND_ACPI_RECLAIMABLE:
                type_desc = "acpi-reclaimable";
                break;
            case MM_MEMMAP_KIND_ACPI_NVS:
                type_desc = "acpi-nvs";
                break;
            case MM_MEMMAP_KIND_BAD_MEMORY:
                type_desc = "bad-memory";
                break;
            case MM_MEMMAP_KIND_BOOTLOADER_RECLAIMABLE:
                // Don't claim bootloader-reclaimable memmaps until after we
                // switch to our own pagemap, because there is a slight chance
                // we allocate the root physical page of the bootloader's
                // page tables.

                type_desc = "bootloader-reclaimable";

                // Because we're claiming this kind of memmap later, they are
                // still represented in the structpage table.

                //structpage_page_count += PAGE_COUNT(memmap->range.size);
                break;
            case MM_MEMMAP_KIND_KERNEL_AND_MODULES:
                type_desc = "kernel-and-modules";
                break;
            case MM_MEMMAP_KIND_FRAMEBUFFER:
                type_desc = "framebuffer";
                break;
        }

        printk(LOGLEVEL_INFO,
               "mm: memmap %" PRIu64 ": [%p-%p] %s\n",
               index + 1,
               (void *)memmap->range.front,
               (void *)range_get_end_assert(memmap->range),
               type_desc);
    }

    printk(LOGLEVEL_INFO,
           "mm: system has %" PRIu64 " usable pages\n",
           total_free_pages);
}

static void init_table_page(struct page *const page) {
    list_init(&page->table.delayed_free_list);
    refcount_init(&page->table.refcount);
}

void
mm_early_refcount_alloced_map(const uint64_t virt_addr, const uint64_t length) {
    struct pt_walker walker;
    ptwalker_create(&walker,
                    virt_addr,
                    /*alloc_pgtable=*/NULL,
                    /*free_pgtable=*/NULL);

    for (pgt_level_t level = (uint8_t)walker.level;
         level <= walker.top_level;
         level++)
    {
        struct page *const page = virt_to_page(walker.tables[level - 1]);
        if (page->table.refcount.count == 0) {
            init_table_page(page);
        }
    }

    // Track changes to the `walker.tables` array by
    //   (1) Seeing if the index of the lowest level ever reaches
    //       (PGT_PTE_COUNT - 1).
    //   (2) Comparing the previous level with the latest level. When the
    //       previous level is greater, the walker has incremented past a
    //       large page, so there may be some tables in between the large page
    //       level and the current level that need to be initialized.

    uint8_t prev_level = walker.level;
    bool prev_was_at_end = walker.indices[prev_level - 1] == PGT_PTE_COUNT - 1;

    for (uint64_t i = 0; i < length;) {
        const enum pt_walker_result advance_result =
            ptwalker_next_with_options(&walker,
                                       walker.level,
                                       /*alloc_parents=*/false,
                                       /*alloc_level=*/false,
                                       /*should_ref=*/false,
                                       /*alloc_pgtable_cb_info=*/NULL,
                                       /*free_pgtable_cb_info=*/NULL);

        if (advance_result != E_PT_WALKER_OK) {
            panic("mm: failed to setup kernel pagemap, result=%d\n",
                  advance_result);
        }

        // When either condition is true, all levels below the highest level
        // incremented will have their index set to 0.
        // To initialize only the tables that have been updated, we start from
        // the current level and ascend to the top level. If an index is
        // non-zero, then we have reached the level that was incremented, and
        // can then exit.

        if (prev_was_at_end || prev_level > walker.level) {
            pgt_level_t level_end = walker.level;
            for (pgt_level_t level = (pgt_level_t)walker.level;
                 level <= walker.top_level;
                 level++)
            {
                const uint64_t index = walker.indices[level - 1];
                if (index != 0) {
                    level_end = level;
                    break;
                }

                pte_t *const table = walker.tables[level - 1];
                struct page *const page = virt_to_page(table);

                init_table_page(page);
            }

            for (pgt_level_t level = (pgt_level_t)walker.level + 1;
                 level <= level_end;
                 level++)
            {
                struct page *const page =
                    virt_to_page(walker.tables[level - 1]);

                ref_up(&page->table.refcount);
            }
        }

        struct page *const page = virt_to_page(walker.tables[walker.level - 1]);

        ref_up(&page->table.refcount);
        i += PAGE_SIZE_AT_LEVEL(walker.level);

        prev_level = walker.level;
        prev_was_at_end = walker.indices[prev_level - 1] == PGT_PTE_COUNT - 1;
    }
}

void mark_used_pages(const struct mm_section *const memmap) {
    struct freepages_info *iter = NULL;
    list_foreach(iter, &g_freepage_list, list) {
        uint64_t iter_phys = virt_to_phys(iter);
        if (!range_has_loc(memmap->range, iter_phys)) {
            continue;
        }

        // Mark the range from the beginning of the memmap, to the first free
        // page.

        struct page *const start = phys_to_page(memmap->range.front);
        struct page *page = start;
        struct page *const unusable_end = virt_to_page(iter);

        for (; page != unusable_end; page++) {
            page->state = PAGE_STATE_NOT_USABLE;
        }

        while (true) {
            // Mark the range from after the free pages, to the end of the
            // original free-page area.

            page += iter->avail_page_count;
            for (uint64_t i = iter->avail_page_count;
                 i != iter->total_page_count;
                 i++, page++)
            {
                page->state = PAGE_STATE_NOT_USABLE;
            }

            iter = list_next(iter, list);
            if (&iter->list == &g_freepage_list) {
                // Mark the range from after the original free-area, to the end
                // of the memmap.

                const struct page *const memmap_end =
                    start + PAGE_COUNT(memmap->range.size);

                for (; page != memmap_end; page++) {
                    page->state = PAGE_STATE_NOT_USABLE;
                }

                return;
            }

            iter_phys = virt_to_phys(iter);
            if (!range_has_loc(memmap->range, iter_phys)) {
                // Mark the range from after the original free-area, to the end
                // of the memmap.

                const struct page *const memmap_end =
                    start + PAGE_COUNT(memmap->range.size);

                for (; page != memmap_end; page++) {
                    page->state = PAGE_STATE_NOT_USABLE;
                }

                return;
            }

            // Mark all pages in between this iterator and the prior as
            // unusable.

            struct page *const end = phys_to_page(iter_phys);
            for (; page != end; page++) {
                page->state = PAGE_STATE_NOT_USABLE;
            }
        }
    }

    // If we couldn't find a corresponding freepages_info struct, then this
    // entire memmap has been used, and needs to be marked as such.

    const uint64_t memmap_page_count = PAGE_COUNT(memmap->range.size);
    struct page *page = phys_to_page(memmap->range.front);

    for (uint64_t i = 0; i != memmap_page_count; i++, page++) {
        page->state = PAGE_STATE_NOT_USABLE;
    }
}

void mark_last_part_of_structpage_table() {
    const struct mm_section *const last_section =
        &mm_get_usable_list()[mm_get_usable_count() - 1];

    struct page *page =
        pfn_to_page(last_section->pfn + PAGE_COUNT(last_section->range.size));

    const struct page *const end = (struct page *)PAGE_END;
    for (; page + sizeof(*page) <= end; page++) {
        page->state = PAGE_STATE_NOT_USABLE;
    }
}

void mark_used_pages_for_bitmap(struct mm_section *const memmap) {
    struct page *page = pfn_to_page(memmap->pfn);
    struct page *end = page + PAGE_COUNT(memmap->range.size);

    uint64_t start_index = 0;
    uint64_t count = 0;

    while (page < end) {
        struct page *begin = page;
        if (page->state == PAGE_STATE_NOT_USABLE) {
            for (; page != end; page++) {
                if (page->state != PAGE_STATE_NOT_USABLE) {
                    break;
                }
            }

            count = (uint64_t)(page - begin);
            const struct range range = range_create(start_index, count);

            bitmap_set_range(&memmap->used_pages_bitmap, range, /*value=*/true);
        } else {
            for (; page != end; page++) {
                if (page->state == PAGE_STATE_NOT_USABLE) {
                    break;
                }
            }

            count = (uint64_t)(page - begin);
        }

        start_index += count;
    }
}

void
set_section_for_pages(const struct mm_section *const memmap,
                      const page_section_t section)
{
    struct freepages_info *iter = NULL;
    list_foreach(iter, &g_freepage_list, list) {
        const uint64_t iter_phys = virt_to_phys(iter);
        if (!range_has_loc(memmap->range, iter_phys)) {
            continue;
        }

        // Mark all usable pages that exist from in iter.
        struct page *page = virt_to_page(iter);
        struct page *const end = page + iter->avail_page_count;

        for (; page != end; page++) {
            page->section = section;
        }
    }
}

static uint64_t free_all_pages() {
    // TODO: Claim acpi-reclaimable pages
    struct freepages_info *iter = NULL;
    struct freepages_info *tmp = NULL;

    /*
     * Free pages into the buddy allocator while ensuring
     *  (1) The range of pages belong to the same zone.
     *  (2) The buddies of the first page for each order from 0...order are
     *      located after the first page.
     */

    uint64_t free_page_count = 0;
    list_foreach_reverse_mut(iter, tmp, &g_asc_freelist, asc_list) {
        uint64_t phys = virt_to_phys(iter);
        uint64_t avail = iter->avail_page_count;

        struct page *page = phys_to_page(phys);

        // iorder is log2() of the count of available pages, here we use a for
        // loop to calculate that, but in the future, it may be worth it to
        // directly use log2() if available.
        // Because the count of available pages only decreases, we keep iorder
        // out of the loop to minimize log2() calculation.

        int8_t iorder = MAX_ORDER - 1;
        do {
            for (; iorder >= 0; iorder--) {
                if (avail >= (1ull << iorder)) {
                    break;
                }
            }

            // jorder is the order of pages that all fit in the same zone.
            // jorder should be equal to iorder in most cases, except in the
            // case where a section crosses the boundary of two zones.

            struct page_zone *const zone = phys_to_zone(phys);
            int8_t jorder = iorder;

            for (; jorder >= 0; jorder--) {
                const uint64_t back_phys =
                    phys + ((PAGE_SIZE << jorder) - PAGE_SIZE);

                if (zone == phys_to_zone(back_phys)) {
                    break;
                }
            }

            const uint64_t total_in_zone = 1ull << jorder;
            uint64_t avail_in_zone = total_in_zone;

            do {
                early_free_pages_to_zone(page, zone, (uint8_t)jorder);
                const uint64_t freed_count = 1ull << jorder;

                page += freed_count;
                avail_in_zone -= freed_count;

                if (avail_in_zone == 0) {
                    break;
                }

                for (; jorder >= 0; jorder--) {
                    if (avail_in_zone >= (1ull << jorder)) {
                        break;
                    }
                }
            } while (true);

            const struct range freed_range = range_create(phys, total_in_zone);
            printk(LOGLEVEL_INFO,
                   "mm: freed %" PRIu64 " pages at " RANGE_FMT " to zone %s\n",
                   total_in_zone,
                   RANGE_FMT_ARGS(freed_range),
                   zone->name);

            avail -= total_in_zone;
            if (avail == 0) {
                break;
            }

            phys += total_in_zone << PAGE_SHIFT;
        } while (true);

        list_delete(&iter->list);
        free_page_count += iter->avail_page_count;
    }

    return free_page_count;
}

void mm_early_post_arch_init() {
#if defined(__aarch64__)
    init_table_page(virt_to_page(kernel_pagemap.lower_root));
    init_table_page(virt_to_page(kernel_pagemap.higher_root));
#else
    init_table_page(virt_to_page(kernel_pagemap.root));
#endif

    // Claim bootloader-reclaimable memmaps now that we've switched to our own
    // pagemap.
    // FIXME: Avoid claiming thees pages until we setup our own stack.

    const uint64_t memmap_count = mm_get_memmap_count();
    for (uint64_t index = 0; index != memmap_count; index++) {
        const struct mm_memmap *const memmap = mm_get_memmap_list() + index;
        if (memmap->kind == MM_MEMMAP_KIND_BOOTLOADER_RECLAIMABLE) {
            //claim_pages(memmap);
        }
    }

    // Iterate over the usable-memmaps (sections) three times:
    //  1. Iterate to mark used-pages first. This must be done first because
    //     it needs to be done before memmaps are merged, as before the merge,
    //     its obvious which pages are used.
    //  2. Set the section mask in page->flags.
    //  3. Setup the bitmap for each memmap. This needs to be separately, and
    //     last, because it needs to call phys_to_page(), which only works after
    //     the section-mask is setup.
    //  4. Protect the pages user for each section's bitmap.

    struct mm_section *const begin = mm_get_usable_list();
    const struct mm_section *const end = begin + mm_get_usable_count();

    for (__auto_type section = begin; section != end; section++) {
        mark_used_pages(section);
    }

    mark_last_part_of_structpage_table();
    boot_merge_usable_memmaps();

    uint64_t number = 0;
    for (__auto_type section = begin; section != end; section++, number++) {
        set_section_for_pages(section, /*section=*/number);
    }

    for (__auto_type section = begin; section != end; section++) {
        const uint64_t alloc_page_count =
            div_round_up(PAGE_COUNT(section->range.size),
                         bytes_to_bits(PAGE_SIZE));

        const uint64_t bitmap_pages_phys =
            early_alloc_multiple_pages(alloc_page_count);
        section->used_pages_bitmap =
            bitmap_open(phys_to_virt(bitmap_pages_phys),
                        alloc_page_count * PAGE_SIZE);

        mark_used_pages_for_bitmap(section);
    }

    for (__auto_type section = begin; section != end; section++) {
        void *const bitmap_begin = section->used_pages_bitmap.gbuffer.begin;
        const void *const bitmap_end = section->used_pages_bitmap.gbuffer.end;

        const uint64_t page_count =
            PAGE_COUNT(distance(bitmap_begin, bitmap_end));

        struct page *const bitmap_page = virt_to_page(bitmap_begin);
        const struct page *bitmap_page_end = bitmap_page + page_count;

        for (struct page *page = bitmap_page; page != bitmap_page_end; page++) {
            page->state = PAGE_STATE_NOT_USABLE;
        }

        struct mm_section *const page_section = page_to_mm_section(bitmap_page);
        const uint64_t start_index =
            (virt_to_phys(bitmap_begin) -
             page_section->range.front) >> PAGE_SHIFT;

        bitmap_set_range(&page_section->used_pages_bitmap,
                         range_create(start_index, page_count),
                         /*value=*/true);
    }

    printk(LOGLEVEL_INFO, "mm: finished setting up structpage table\n");
    pagezones_init();

    const uint64_t free_page_count = free_all_pages();
    for_each_page_zone(zone) {
        printk(LOGLEVEL_INFO,
               "mm: zone %s has %" PRIu64 " pages\n",
               zone->name,
               zone->total_free);
    }

    printk(LOGLEVEL_INFO,
           "mm: system has %" PRIu64 " free pages\n",
           free_page_count);

    kmalloc_init();
    mm_alloc_init();
}