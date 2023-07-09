/*
 * kernel/arch/x86_64/mm/init.c
 * © suhas pai
 */

#include "asm/regs.h"
#include "lib/align.h"
#include "dev/printk.h"

#include "mm/kmalloc.h"
#include "mm/page_alloc.h"
#include "mm/walker.h"

#include "limine.h"

volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0,
    .response = NULL
};

static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0,
    .response = NULL
};

volatile struct limine_paging_mode_request paging_mode_request = {
    .id = LIMINE_PAGING_MODE_REQUEST,
    .revision = 0,
    .response = NULL
};

struct freepages_info {
    struct list list_entry;

    /* Count of contiguous pages, starting from this one, that are free */
    uint64_t count;
};

static struct list freepages_list = LIST_INIT(freepages_list);
static uint64_t total_free_pages = 0;

static void claim_pages(const uint64_t first_phys, const uint64_t count) {
    // Check if the we can combine this entry and the prior one.
    struct freepages_info *const back =
        list_tail(&freepages_list, struct freepages_info, list_entry);

    // Store structure in the first page in list.
    struct freepages_info *const info = phys_to_virt(first_phys);
    total_free_pages += count;

    if (back != NULL) {
        if ((uint8_t *)back + (back->count * PAGE_SIZE) == (uint8_t *)info) {
            back->count += count;
            return;
        }
    }

    info->count = count;
    list_add(&freepages_list, &info->list_entry);
}

static uint64_t early_alloc_page() {
    if (list_empty(&freepages_list)) {
        printk(LOGLEVEL_ERROR, "Ran out of free-pages\n");
        return 0;
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

    memzero(phys_to_virt(free_page), PAGE_SIZE);
    return free_page;
}

__unused static uint64_t alloc_cont_pages(const uint32_t amount) {
    if (list_empty(&freepages_list)) {
        printk(LOGLEVEL_ERROR, "Ran out of free-pages\n");
        return 0;
    }

    struct freepages_info *info = NULL;

    uint64_t free_page = 0;
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

    if (free_page == 0) {
        return 0;
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

    memzero(phys_to_virt(free_page), PAGE_SIZE * amount);
    return free_page;
}

static struct page *
ptwalker_alloc_pgtable_cb(struct pt_walker *const walker, void *const cb_info) {
    (void)walker;
    (void)cb_info;

    // We don't have a structpage-table setup yet when this function is called,
    // but because ptwalker never dereferences the page, we can return a pointer
    // to where the page would've been.

    const uint64_t phys = early_alloc_page();
    if (phys != 0) {
        return phys_to_page(phys);
    }

    panic("Failed to setup page-structs. Ran out of memory\n");
}

static void
setup_pagestructs_table(const uint64_t root_page, uint64_t byte_count) {
    const uint64_t structpage_count = PAGE_COUNT(byte_count);
    uint64_t map_size = structpage_count * SIZEOF_STRUCTPAGE;

    if (!align_up(map_size, PAGE_SIZE_2MIB, &map_size)) {
        panic("Failed to initialize memory, overflow error when alighing");
    }

    printk(LOGLEVEL_INFO,
           "mm: structpage table is %" PRIu64 " bytes\n",
           map_size);

    // Map struct page table
    struct pt_walker pt_walker = {};
    const uint64_t pte_flags =
        __PTE_PRESENT | __PTE_WRITE | __PTE_GLOBAL | __PTE_NOEXEC;

    enum pt_walker_result walker_result = E_PT_WALKER_OK;
    ptwalker_create_customroot(&pt_walker,
                               root_page,
                               PAGE_OFFSET,
                               /*alloc_pgtable=*/ptwalker_alloc_pgtable_cb,
                               /*free_pgtable=*/NULL);

    if (map_size >= PAGE_SIZE_1GIB) {
        walker_result =
            ptwalker_fill_in_to(&pt_walker,
                                /*level=*/3,
                                /*should_ref=*/false,
                                /*alloc_pgtable_cb_info=*/NULL,
                                /*free_pgtable_cb_info=*/NULL);

        if (walker_result != E_PT_WALKER_OK) {
        panic:
            panic("Failed to setup page-structs. Ran out of memory\n");
        }

        do {
            const uint64_t page =
                alloc_cont_pages(/*order=*/PGT_COUNT * PGT_COUNT);

            if (page == 0) {
                // We failed to alloc a 1gib page, so try 2mib pages next.
                break;
            }

            *ptwalker_pte_in_level(&pt_walker, /*level=*/3) =
                page | pte_flags | __PTE_LARGE;

            walker_result =
                ptwalker_next_custom(&pt_walker,
                                     /*level=*/3,
                                     /*alloc_parents=*/true,
                                     /*alloc_level=*/true,
                                     /*should_ref=*/false,
                                     /*alloc_pgtable_cb_info=*/NULL,
                                     /*free_pgtable_cb_info=*/NULL);

            if (walker_result != E_PT_WALKER_OK) {
                goto panic;
            }

            map_size -= PAGE_SIZE_1GIB;
        } while (map_size >= PAGE_SIZE_1GIB);
    }

    if (map_size >= PAGE_SIZE_2MIB) {
        walker_result =
            ptwalker_fill_in_to(&pt_walker,
                                /*level=*/2,
                                /*should_ref=*/false,
                                /*alloc_pgtable_cb_info=*/NULL,
                                /*free_pgtable_cb_info=*/NULL);

        if (walker_result != E_PT_WALKER_OK) {
            goto panic;
        }

        do {
            const uint64_t page = alloc_cont_pages(/*order=*/PGT_COUNT);
            if (page == 0) {
                // We failed to alloc a 2mib page, so fill with 4kib pages
                // instead.
                break;
            }

            *ptwalker_pte_in_level(&pt_walker, /*level=*/2) =
                page | pte_flags | __PTE_LARGE;

            walker_result =
                ptwalker_next_custom(&pt_walker,
                                     /*level=*/2,
                                     /*alloc_parents=*/true,
                                     /*alloc_level=*/true,
                                     /*should_ref=*/false,
                                     /*alloc_pgtable_cb_info=*/NULL,
                                     /*free_pgtable_cb_info=*/NULL);

            if (walker_result != E_PT_WALKER_OK) {
                goto panic;
            }

            map_size -= PAGE_SIZE_2MIB;
        } while (map_size >= PAGE_SIZE_2MIB);
    }

    if (map_size >= PAGE_SIZE) {
        walker_result =
            ptwalker_fill_in_to(&pt_walker,
                                /*level=*/1,
                                /*should_ref=*/false,
                                /*alloc_pgtable_cb_info=*/NULL,
                                /*free_pgtable_cb_info=*/NULL);

        if (walker_result != E_PT_WALKER_OK) {
            goto panic;
        }

        do {
            const uint64_t page = early_alloc_page();
            if (page == 0) {
                // We failed to alloc a 2mib page, so fill with 4kib pages
                // instead.
                break;
            }

            *ptwalker_pte_in_level(&pt_walker, /*level=*/1) =
                page | pte_flags;

            walker_result =
                ptwalker_next_custom(&pt_walker,
                                     /*level=*/1,
                                     /*alloc_parents=*/true,
                                     /*alloc_level=*/true,
                                     /*should_ref=*/false,
                                     /*alloc_pgtable_cb_info=*/NULL,
                                     /*free_pgtable_cb_info=*/NULL);

            if (walker_result != E_PT_WALKER_OK) {
                goto panic;
            }

            map_size -= PAGE_SIZE;
        } while (map_size >= PAGE_SIZE);
    }
}

static inline bool is_usable_memory(const uint64_t type) {
    return type == LIMINE_MEMMAP_USABLE ||
           type == LIMINE_MEMMAP_ACPI_RECLAIMABLE ||
           type == LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE;
}

void mm_init() {
    assert(hhdm_request.response != NULL);
    assert(memmap_request.response != NULL);
    assert(paging_mode_request.response != NULL);

    printk(LOGLEVEL_INFO,
           "mm: hhdm at %p\n",
           (void *)hhdm_request.response->offset);

    const struct limine_memmap_response *const resp = memmap_request.response;

    struct limine_memmap_entry *const *entries = resp->entries;
    struct limine_memmap_entry *const *const end = entries + resp->entry_count;

    uint64_t memmap_index = 0;
    uint64_t memmap_last_usable_index = 0;

    for (__auto_type memmap_iter = entries; memmap_iter != end; memmap_iter++) {
        const struct limine_memmap_entry *const memmap = *memmap_iter;
        const char *type_desc = "<unknown>";

        // Don't yet claim pages in *Reclaimable memmaps, although we do map it
        // in the structpage table.

        switch (memmap->type) {
            case LIMINE_MEMMAP_USABLE:
                claim_pages(memmap->base, PAGE_COUNT(memmap->length));
                type_desc = "usable";

                break;
            case LIMINE_MEMMAP_RESERVED:
                type_desc = "reserved";
                break;
            case LIMINE_MEMMAP_ACPI_RECLAIMABLE:
                type_desc = "acpi-reclaimable";
                break;
            case LIMINE_MEMMAP_ACPI_NVS:
                type_desc = "acpi-nvs";
                break;
            case LIMINE_MEMMAP_BAD_MEMORY:
                type_desc = "bad-memory";
                break;
            case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE:
                type_desc = "bootloader-reclaimable";
                break;
            case LIMINE_MEMMAP_KERNEL_AND_MODULES:
                type_desc = "kernel-and-modules";
                break;
            case LIMINE_MEMMAP_FRAMEBUFFER:
                type_desc = "framebuffer";
                break;
        }

        if (is_usable_memory(memmap->type)) {
            memmap_last_usable_index = memmap_index;
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

    const struct limine_memmap_entry *const last_usable_memmap =
        entries[memmap_last_usable_index];

    const uint64_t total_bytes_repr_by_structpage_table =
        (last_usable_memmap->base + last_usable_memmap->length) -
        entries[0]->base;

    setup_pagestructs_table(read_cr3(), total_bytes_repr_by_structpage_table);

    const struct limine_memmap_entry *const first_memmap = entries[0];
    if (first_memmap->base != 0) {
        struct page *page = phys_to_page(0);
        for (uint64_t j = 0; j != PAGE_COUNT(first_memmap->base); j++, page++) {
            // Avoid using `set_page_bit()` because we don't need atomic ops
            // this early.

            page->flags |= PAGE_NOT_USABLE;
        }
    }

    for (uint64_t i = 0; i != memmap_last_usable_index; i++) {
        struct limine_memmap_entry *const memmap = entries[i];
        if (is_usable_memory(memmap->type)) {
            continue;
        }

        struct page *page = phys_to_page(memmap->base);
        for (uint64_t j = 0; j != PAGE_COUNT(memmap->length); j++, page++) {
            // Avoid using `set_page_bit()` because we don't need atomic ops
            // this early.

            page->flags |= PAGE_NOT_USABLE;
        }
    }

    printk(LOGLEVEL_INFO, "mm: finished setting up structpage table\n");
    pagezones_init();

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
    printk(LOGLEVEL_INFO, "mm: finished setting up\n");
}