/*
 * kernel/mm/early.c
 * © suhas pai
 */

#include "dev/printk.h"
#include "lib/align.h"
#include "lib/math.h"
#include "mm/walker.h"

#include "limine.h"
#include "kmalloc.h"

#include "page_alloc.h"
#include "page.h"

static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0,
    .response = NULL
};

static volatile struct limine_kernel_address_request kern_addr_request = {
    .id = LIMINE_KERNEL_ADDRESS_REQUEST,
    .revision = 0,
    .response = NULL
};

volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0,
    .response = NULL
};

struct freepages_info {
    struct list list_entry;

    /* Count of contiguous pages, starting from this one, that are free */
    uint64_t count;
};

static struct list freepages_list = LIST_INIT(freepages_list);
static uint64_t freepages_list_count = 0;
static uint64_t total_free_pages = 0;

static void claim_pages(const uint64_t first_phys, const uint64_t length) {
    const uint64_t page_count = PAGE_COUNT(length);

    // Check if the we can combine this entry and the prior one.
    // Store structure in the first page in list.
    struct freepages_info *const info = phys_to_virt(first_phys);
    total_free_pages += page_count;

    if (freepages_list_count != 0) {
        struct freepages_info *const back =
            list_tail(&freepages_list, struct freepages_info, list_entry);

        if ((uint8_t *)back + (back->count * PAGE_SIZE) == (uint8_t *)info) {
            back->count += page_count;
            return;
        }
    }

    info->count = page_count;

    list_add(&freepages_list, &info->list_entry);
    freepages_list_count++;
}

uint64_t early_alloc_page() {
    if (list_empty(&freepages_list)) {
        printk(LOGLEVEL_ERROR, "Ran out of free-pages\n");
        return INVALID_PHYS;
    }

    struct freepages_info *const info =
        list_head(&freepages_list, struct freepages_info, list_entry);

    // Take last page out of list, because first page stores the freepage_info
    // struct.

    info->count -= 1;
    const uint64_t free_page = virt_to_phys(info) + (info->count * PAGE_SIZE);

    if (info->count == 0) {
        list_delete(&info->list_entry);
    }

    bzero(phys_to_virt(free_page), PAGE_SIZE);
    return free_page;
}

uint64_t early_alloc_cont_pages(const uint32_t amount) {
    if (list_empty(&freepages_list)) {
        printk(LOGLEVEL_ERROR, "Ran out of free-pages\n");
        return INVALID_PHYS;
    }

    struct freepages_info *info = NULL;

    uint64_t free_page = INVALID_PHYS;
    bool is_in_middle = false;

    list_foreach(info, &freepages_list, list_entry) {
        uint64_t count = info->count;
        if (count < amount) {
            continue;
        }

        uint64_t phys = virt_to_phys(info);
        uint64_t last_phys = phys + ((count - amount) * PAGE_SIZE);

        if (!has_align(last_phys, PAGE_SIZE * amount)) {
            last_phys = align_down(last_phys, PAGE_SIZE * amount);
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
        const uint64_t old_info_count = info->count;

        info->count = PAGE_COUNT(free_page - virt_to_phys(info));
        if (info->count == 0) {
            prev = list_prev(info, list_entry);
            list_delete(&info->list_entry);
        }

        const uint64_t new_info_phys = free_page + (PAGE_SIZE * amount);
        const uint64_t new_info_count = old_info_count - info->count - amount;

        if (new_info_count != 0) {
            struct freepages_info *const new_info =
                (struct freepages_info *)phys_to_virt(new_info_phys);

            new_info->count = new_info_count;
            list_add(&prev->list_entry, &new_info->list_entry);
        }
    } else {
        info->count -= amount;
        if (info->count == 0) {
            list_delete(&info->list_entry);
        }
    }

    bzero(phys_to_virt(free_page), PAGE_SIZE * amount);
    return free_page;
}

uint64_t KERNEL_BASE = 0;
uint64_t memmap_last_repr_index = 0;

void mm_early_init() {
    assert(hhdm_request.response != NULL);
    assert(kern_addr_request.response != NULL);
    assert(memmap_request.response != NULL);

    HHDM_OFFSET = hhdm_request.response->offset;
    KERNEL_BASE = kern_addr_request.response->virtual_base;

    printk(LOGLEVEL_INFO, "mm: hhdm at %p\n", (void *)HHDM_OFFSET);
    printk(LOGLEVEL_INFO, "mm: kernel at %p\n", (void *)KERNEL_BASE);

    const struct limine_memmap_response *const resp = memmap_request.response;

    struct limine_memmap_entry *const *entries = resp->entries;
    struct limine_memmap_entry *const *const end = entries + resp->entry_count;

    uint64_t memmap_index = 0;
    for (__auto_type memmap_iter = entries; memmap_iter != end; memmap_iter++) {
        const struct limine_memmap_entry *const memmap = *memmap_iter;
        const char *type_desc = "<unknown>";

        // Don't claim pages in *Reclaimable memmaps yet, although we do map it
        // in the structpage table.

        switch (memmap->type) {
            case LIMINE_MEMMAP_USABLE:
                claim_pages(memmap->base, memmap->length);

                type_desc = "usable";
                memmap_last_repr_index = memmap_index;

                break;
            case LIMINE_MEMMAP_RESERVED:
                type_desc = "reserved";
                break;
            case LIMINE_MEMMAP_ACPI_RECLAIMABLE:
                type_desc = "acpi-reclaimable";
                memmap_last_repr_index = memmap_index;

                break;
            case LIMINE_MEMMAP_ACPI_NVS:
                type_desc = "acpi-nvs";
                memmap_last_repr_index = memmap_index;

                break;
            case LIMINE_MEMMAP_BAD_MEMORY:
                type_desc = "bad-memory";
                break;
            case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE:
                type_desc = "bootloader-reclaimable";
                memmap_last_repr_index = memmap_index;

                break;
            case LIMINE_MEMMAP_KERNEL_AND_MODULES:
                type_desc = "kernel-and-modules";
                memmap_last_repr_index = memmap_index;

                break;
            case LIMINE_MEMMAP_FRAMEBUFFER:
                type_desc = "framebuffer";
                memmap_last_repr_index = memmap_index;

                break;
        }

        printk(LOGLEVEL_INFO,
               "mm: memmap %" PRIu64 ": [%p-%p] %s\n",
               memmap_index + 1,
               (void *)memmap->base,
               (void *)(memmap->base + memmap->length),
               type_desc);

        memmap_index++;
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
    struct pt_walker walker = {0};
    ptwalker_create(&walker,
                    virt_addr,
                    /*alloc_pgtable=*/NULL,
                    /*free_pgtable=*/NULL);

    for (uint8_t level = (uint8_t)walker.level;
         level <= walker.top_level;
         level++)
    {
        struct page *const page = virt_to_page(walker.tables[level - 1]);
        init_table_page(page);
    }

    // Track changes to the `walker.tables` array by
    //   (1) Seeing if the index of the lowest level ever reaches
    //       (PGT_COUNT - 1).
    //   (2) Comparing the previous level with the latest level. When the
    //       previous level is greater, the walker has incremented past a
    //       large page, so there may be some tables in between the large page
    //       level and the current level that need to be initialized.

    uint8_t prev_level = walker.level;
    bool prev_was_at_end = 0;

    for (uint64_t i = 0; i < length;) {
        const enum pt_walker_result advance_result =
            ptwalker_next_with_options(&walker,
                                       /*level=*/walker.level,
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
            for (uint8_t level = (uint8_t)walker.level;
                 level < walker.top_level;
                 level++)
            {
                const uint64_t index = walker.indices[level - 1];
                if (index != 0) {
                    break;
                }

                pte_t *const table = walker.tables[level - 1];
                struct page *const page = pte_to_page(*table);

                init_table_page(page);
            }
        }

        uint64_t page_size = PAGE_SIZE;
        for (uint8_t j = 1; j != walker.level; j++) {
            page_size *= PGT_COUNT;
        }

        i += page_size;
        prev_level = walker.level;
        prev_was_at_end = walker.indices[walker.level - 1] == PGT_COUNT - 1;
    }
}

void mm_early_post_arch_init() {
    const struct limine_memmap_response *const resp = memmap_request.response;

    struct limine_memmap_entry *const *entries = resp->entries;
    const struct limine_memmap_entry *const first_memmap = entries[0];

    if (first_memmap->base != 0) {
        struct page *page = phys_to_page(0);
        for (uint64_t j = 0; j != PAGE_COUNT(first_memmap->base); j++, page++) {
            // Avoid using `set_page_bit()` because we don't need atomic ops
            // this early.

            page->flags = PAGE_NOT_USABLE;
        }
    }

    for (uint64_t i = 0; i <= memmap_last_repr_index; i++) {
        struct limine_memmap_entry *const memmap = entries[i];
        if (memmap->type == LIMINE_MEMMAP_USABLE) {
            continue;
        }

        struct page *page = phys_to_page(memmap->base);
        for (uint64_t j = 0; j != PAGE_COUNT(memmap->length); j++, page++) {
            // Avoid using `set_page_bit()` because we don't need atomic ops
            // this early.

            page->flags = PAGE_NOT_USABLE;
        }
    }

    printk(LOGLEVEL_INFO, "mm: finished setting up structpage table\n");
    pagezones_init();

    // TODO: Claim bootloader-reclaimable and acpi-reclaimable pages
    struct freepages_info *walker = NULL;
    struct freepages_info *tmp = NULL;

    list_foreach_mut(walker, tmp, &freepages_list, list_entry) {
        list_delete(&walker->list_entry);

        struct page *page = virt_to_page(walker);
        const struct page *const page_end = page + walker->count;

        for (; page != page_end; page++) {
            free_page(page);
        }
    }

    kmalloc_init();
}