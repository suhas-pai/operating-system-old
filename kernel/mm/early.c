/*
 * kernel/mm/early.c
 * © suhas pai
 */

#include "dev/printk.h"
#include "lib/align.h"

#include "boot.h"
#include "early.h"

#include "pagemap.h"
#include "walker.h"

#include "kmalloc.h"

struct freepages_info {
    struct list list;

    // Count of contiguous pages, starting from this one
    uint64_t total_page_count;
    // Number of available pages in this freepages_info struct.
    uint64_t avail_page_count;
} __page_aligned;

_Static_assert(sizeof(struct freepages_info) <= PAGE_SIZE,
               "freepages_info struct must be small enough to store on a "
               "single page");

static struct list freepages_list = LIST_INIT(freepages_list);
static uint64_t freepages_list_count = 0;
static uint64_t total_free_pages = 0;
static uint64_t total_free_pages_remaining = 0;

static void claim_pages(const struct mm_memmap *const memmap) {
    const uint64_t page_count = PAGE_COUNT(memmap->range.size);

    // Check if the we can combine this entry and the prior one.
    // Store structure in the first page in list.

    struct freepages_info *const info = phys_to_virt(memmap->range.front);
    list_init(&info->list);

    total_free_pages += page_count;
    total_free_pages_remaining += page_count;

    info->avail_page_count = page_count;
    info->total_page_count = page_count;

    freepages_list_count++;
    if (freepages_list_count != 1) {
        struct freepages_info *prev =
            list_tail(&freepages_list, struct freepages_info, list);

        if (prev > info) {
            prev = container_of(&freepages_list, struct freepages_info, list);
            struct freepages_info *iter = NULL;

            list_foreach(iter, &freepages_list, list) {
                if (info < iter) {
                    break;
                }

                prev = iter;
            }

            if (prev->list.next != &freepages_list) {
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
        }

        if (&prev->list != &freepages_list) {
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
        return;
    }

    list_add(&freepages_list, &info->list);
}

uint64_t early_alloc_page() {
    if (__builtin_expect(list_empty(&freepages_list), 0)) {
        printk(LOGLEVEL_ERROR, "Ran out of free-pages\n");
        return INVALID_PHYS;
    }

    struct freepages_info *const info =
        list_head(&freepages_list, struct freepages_info, list);

    // Take last page out of list, because first page stores the freepage_info
    // struct.

    info->avail_page_count -= 1;
    if (info->avail_page_count == 0) {
        list_delete(&info->list);
    }

    const uint64_t free_page =
        virt_to_phys(info) + (info->avail_page_count * PAGE_SIZE);

    zero_page(phys_to_virt(free_page));
    total_free_pages_remaining--;

    return free_page;
}

uint64_t early_alloc_multiple_pages(const uint64_t alloc_amount) {
    if (__builtin_expect(list_empty(&freepages_list), 0)) {
        printk(LOGLEVEL_ERROR, "Ran out of free-pages\n");
        return INVALID_PHYS;
    }

    struct freepages_info *info = NULL;
    list_foreach(info, &freepages_list, list) {
        const uint64_t avail_page_count = info->avail_page_count;
        if (avail_page_count >= alloc_amount) {
            break;
        }
    }

    if (info == NULL) {
        return INVALID_PHYS;
    }

    // Take last page out of list, because first page stores the freepage_info
    // struct.

    info->avail_page_count -= alloc_amount;
    if (info->avail_page_count == 0) {
        list_delete(&info->list);
    }

    const uint64_t free_page =
        virt_to_phys(info) + (info->avail_page_count * PAGE_SIZE);

    zero_page(phys_to_virt(free_page));
    total_free_pages_remaining -= alloc_amount;

    return free_page;
}

uint64_t early_alloc_large_page(const uint32_t alloc_amount) {
    if (__builtin_expect(list_empty(&freepages_list), 0)) {
        printk(LOGLEVEL_ERROR, "Ran out of free-pages\n");
        return INVALID_PHYS;
    }

    struct freepages_info *info = NULL;

    uint64_t free_page = INVALID_PHYS;
    bool is_in_middle = false;

    list_foreach(info, &freepages_list, list) {
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

        if (info->avail_page_count == 0) {
            prev = list_prev(info, list);
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
            list_add(&prev->list, &new_info->list);
        }
    } else {
        info->avail_page_count -= alloc_amount;
        if (info->avail_page_count == 0) {
            list_delete(&info->list);
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

    for (uint64_t index = 0; index != mm_get_memmap_count(); index++) {
        const struct mm_memmap *const memmap = mm_get_memmap_list() + index;
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

                structpage_page_count += PAGE_COUNT(memmap->range.size);
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
    list_foreach(iter, &freepages_list, list) {
        uint64_t iter_phys = virt_to_phys(iter);
        if (!range_has_loc(memmap->range, iter_phys)) {
            continue;
        }

        struct page *page = phys_to_page(memmap->range.front);
        struct page *const unusable_end = virt_to_page(iter);

        for (; page != unusable_end; page++) {
            page->flags = PAGE_NOT_USABLE;
        }

        while (true) {
            page += iter->avail_page_count;

            // Mark all pages as non-usable that exist from after iter.
            for (uint64_t i = iter->avail_page_count;
                 i != iter->total_page_count;
                 i++, page++)
            {
                page->flags = PAGE_NOT_USABLE;
            }

            iter = list_next(iter, list);
            if (&iter->list == &freepages_list) {
                return;
            }

            iter_phys = virt_to_phys(iter);
            if (!range_has_loc(memmap->range, iter_phys)) {
                return;
            }

            // Mark all pages in between this iterator and the prior as
            // unusable.

            struct page *const end = phys_to_page(iter_phys);
            for (; page != end; page++) {
                page->flags = PAGE_NOT_USABLE;
            }
        }
    }

    // If we couldn't find a corresponding freepages_info struct, then this
    // entire memmap has been used, and needs to be marked as such.

    const uint64_t memmap_page_count = PAGE_COUNT(memmap->range.size);
    struct page *page = phys_to_page(memmap->range.front);

    for (uint64_t i = 0; i != memmap_page_count; i++, page++) {
        page->flags = PAGE_NOT_USABLE;
    }
}

void mark_used_pages_for_bitmap(struct mm_section *const memmap) {
    struct page *page = pfn_to_page(memmap->pfn);
    struct page *end = page + PAGE_COUNT(memmap->range.size);

    uint64_t start_index = 0;
    uint64_t count = 0;

    while (page < end) {
        struct page *begin = page;
        if (page->flags == PAGE_NOT_USABLE) {
            for (; page != end; page++) {
                if (page->flags != PAGE_NOT_USABLE) {
                    break;
                }
            }

            count = (uint64_t)(page - begin);
            const struct range range = range_create(start_index, count);

            bitmap_set_range(&memmap->used_pages_bitmap, range, /*value=*/true);
        } else {
            for (; page != end; page++) {
                if (page->flags == PAGE_NOT_USABLE) {
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
    list_foreach(iter, &freepages_list, list) {
        const uint64_t iter_phys = virt_to_phys(iter);
        if (!range_has_loc(memmap->range, iter_phys)) {
            continue;
        }

        const uint64_t pages_in_this_section =
            PAGE_COUNT(memmap->range.size -
                       range_index_for_loc(memmap->range, iter_phys));

        // Mark all usable pages that exist from in iter.
        struct page *page = virt_to_page(iter);
        struct page *const end =
            page + min(iter->avail_page_count, pages_in_this_section);

        for (; page != end; page++) {
            page->flags = (section & SECTION_MASK) << SECTION_SHIFT;
        }
    }
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

    for (uint64_t index = 0; index != mm_get_memmap_count(); index++) {
        const struct mm_memmap *const memmap = mm_get_memmap_list() + index;
        if (memmap->kind == MM_MEMMAP_KIND_BOOTLOADER_RECLAIMABLE) {
            claim_pages(memmap);
        }
    }

    // Iterate over the usable-memmaps (sections) three times.
    //  1. Iterate over the used-pages first. This must be done first because
    //     it needs to be before memmaps are merged, as before the merge, its
    //     obvious which pages are used.
    //  2. Set the section mask in page->flags.
    //  3. Setup the bitmap for each memmap. This needs to be separately, and
    //     last, because it needs to call phys_to_page(), which only works after
    //     the section-mask is setup.
    //  4. Protect the pages user for each section's bitmap.

    for (uint64_t index = 0; index != mm_get_usable_count(); index++) {
        mark_used_pages(mm_get_usable_list() + index);
    }

    boot_merge_usable_memmaps();
    for (uint64_t index = 0; index != mm_get_usable_count(); index++) {
        struct mm_section *const section = mm_get_usable_list() + index;
        set_section_for_pages(section, /*section=*/index);
    }

    for (uint64_t index = 0; index != mm_get_usable_count(); index++) {
        struct mm_section *const section = mm_get_usable_list() + index;
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

    for (uint64_t index = 0; index != mm_get_usable_count(); index++) {
        struct mm_section *const section = mm_get_usable_list() + index;

        void *const bitmap_begin = section->used_pages_bitmap.gbuffer.begin;
        const uint64_t page_count =
            PAGE_COUNT(
                distance(bitmap_begin, section->used_pages_bitmap.gbuffer.end));

        struct page *page = virt_to_page(bitmap_begin);
        const struct page *end = page + page_count;

        for (; page != end; page++) {
            page->flags = PAGE_NOT_USABLE;
        }

        struct mm_section *const page_section = page_to_mm_section(page);
        const uint64_t start_index =
            (virt_to_phys(bitmap_begin) -
             page_section->range.front) >> PAGE_SHIFT;

        bitmap_set_range(&page_section->used_pages_bitmap,
                         range_create(start_index, page_count),
                         /*value=*/true);
    }

    printk(LOGLEVEL_INFO, "mm: finished setting up structpage table\n");
    pagezones_init();

    // TODO: Claim acpi-reclaimable pages
    struct freepages_info *iter = NULL;
    struct freepages_info *tmp = NULL;

    uint64_t free_page_count = 0;
    list_foreach_mut(iter, tmp, &freepages_list, list) {
        list_delete(&iter->list);

        struct page *page = virt_to_page(iter);
        const struct page *const page_end = page + iter->avail_page_count;

        for (; page != page_end; page++) {
            early_free_pages_to_zone(page, page_to_zone(page), /*order=*/0);
        }

        free_page_count += iter->avail_page_count;
    }

    printk(LOGLEVEL_INFO,
           "mm: system has %" PRIu64 " free pages\n",
           free_page_count);

    kmalloc_init();
}