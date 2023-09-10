/*
 * kernek/boot.c
 * © suhas pai
 */

#include "dev/printk.h"
#include "lib/assert.h"
#include "mm/mm_types.h"

#include "boot.h"
#include "limine.h"

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

static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0,
    .response = NULL
};

static volatile struct limine_paging_mode_request paging_mode_request = {
    .id = LIMINE_PAGING_MODE_REQUEST,
    .revision = 0,
    .response = NULL
};

static volatile struct limine_rsdp_request rsdp_request = {
    .id = LIMINE_RSDP_REQUEST,
    .revision = 0
};

static volatile struct limine_dtb_request dtb_request = {
    .id = LIMINE_DTB_REQUEST,
    .revision = 0,
    .response = NULL
};

static volatile struct limine_boot_time_request boot_time_request = {
    .id = LIMINE_BOOT_TIME_REQUEST,
    .revision = 0
};

static struct mm_memmap mm_memmap_list[255] = {0};
static uint8_t mm_memmap_count = 0;

// If we have a memmap whose memory pages will go in the structpage
// table, then they will also be stored in the mm_usable_list array.
//
// We store twice so the functions `phys_to_pfn()` can efficiently find
// the memmap that corresponds to a physical address w/o having to traverse the
// entire memmap-list, which is made up mostly of memmaps not mapped into the
// structpage-table

static struct mm_memmap mm_usable_list[255] = {0};
static uint8_t mm_usable_count = 0;

static const void *rsdp = NULL;
static const void *dtb = NULL;

static int64_t boot_time = 0;

const struct mm_memmap *mm_get_memmap_list() {
    return mm_memmap_list;
}

const struct mm_memmap *mm_get_usable_list() {
    return mm_usable_list;
}

struct mm_memmap *mm_get_usable_list_mut() {
    return mm_usable_list;
}

uint8_t mm_get_memmap_count() {
    return mm_memmap_count;
}

uint8_t mm_get_usable_count() {
    return mm_usable_count;
}

const void *boot_get_rsdp() {
    return rsdp;
}

const void *boot_get_dtb() {
    return dtb;
}

int64_t boot_get_time() {
    return boot_time;
}

void boot_early_init() {
    if (dtb_request.response == NULL ||
        dtb_request.response->dtb_ptr == NULL)
    {
    #if defined(__riscv64)
        panic("boot: device tree not found");
    #else
        printk(LOGLEVEL_WARN, "boot: device tree is missing\n");
    #endif /* defined(__riscv64) */
    } else {
        dtb = dtb_request.response->dtb_ptr;
    }
}

void boot_init() {
    assert(hhdm_request.response != NULL);
    assert(kern_addr_request.response != NULL);
    assert(memmap_request.response != NULL);
    assert(paging_mode_request.response != NULL);

    HHDM_OFFSET = hhdm_request.response->offset;
    KERNEL_BASE = kern_addr_request.response->virtual_base;
    PAGING_MODE = paging_mode_request.response->mode;

    const struct limine_memmap_response *const resp = memmap_request.response;

    struct limine_memmap_entry *const *entries = resp->entries;
    struct limine_memmap_entry *const *const end = entries + resp->entry_count;

    uint8_t memmap_index = 0;
    uint8_t usable_index = 0;

    // Usable (and bootloader-reclaimable) memmaps are sparsely located in the
    // physical memory-space, but are stored together (contiguously) in the
    // structpage-table in virtual-memory.
    // To accomplish this, assign each memmap placed in the structpage-table a
    // pfn that will be assigned to the first page residing in the memmap.

    uint64_t pfn = 0;
    for (__auto_type memmap_iter = entries; memmap_iter != end; memmap_iter++) {
        if (memmap_index == countof(mm_memmap_list)) {
            panic("boot: too many memmaps\n");
        }

        const struct limine_memmap_entry *const memmap = *memmap_iter;
        struct range range = RANGE_EMPTY();

        if (!range_align_out(range_create(memmap->base, memmap->length),
                             /*boundary=*/PAGE_SIZE,
                             &range))
        {
            panic("boot: failed to align memmap");
        }

        // If we find overlapping memmaps, try and fix the range of our current
        // memmap based on the range of the previous memmap. We assume the
        // memory-maps are sorted in ascending order.

        if (memmap_index != 0) {
            const struct range prev_range =
                mm_memmap_list[memmap_index - 1].range;

            if (range_overlaps(prev_range, range)) {
                if (range_has(prev_range, range)) {
                    // If the previous memmap completely contains this memmap,
                    // then simply skip this memmap.

                    continue;
                }

                const uint64_t prev_end = range_get_end_assert(prev_range);
                range = range_from_loc(range, prev_end);
            }
        }

        mm_memmap_list[memmap_index] = (struct mm_memmap){
            .range = range,
            .kind = (enum mm_memmap_kind)(memmap->type + 1)
        };

        if (memmap->type == LIMINE_MEMMAP_USABLE ||
            memmap->type == LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE)
        {
            mm_memmap_list[memmap_index].pfn = pfn;
            mm_usable_list[usable_index] = mm_memmap_list[memmap_index];

            pfn += PAGE_COUNT(range.size);
            usable_index++;
        }

        memmap_index++;
    }

    mm_memmap_count = memmap_index;
    mm_usable_count = usable_index;

    printk(LOGLEVEL_INFO,
           "boot: there are %" PRIu8 " usable memmaps\n",
           mm_usable_count);

    if (rsdp_request.response == NULL ||
        rsdp_request.response->address == NULL)
    {
    #if !defined(__riscv64)
        panic("boot: acpi not found\n");
    #else
        printk(LOGLEVEL_WARN, "boot: acpi does not exist\n");
    #endif /* !defined(__riscv64) */
    } else {
        rsdp = rsdp_request.response->address;
    }

    if (boot_time_request.response == NULL) {
        panic("boot: boot-time not found\n");
    }

    boot_time = boot_time_request.response->boot_time;
    printk(LOGLEVEL_INFO, "boot: boot timestamp is %" PRIu64 "\n", boot_time);
}

void boot_merge_usable_memmaps() {
    for (uint64_t index = 1; index != mm_usable_count;) {
        const struct mm_memmap *const memmap = mm_usable_list + index;
        do {
            const uint64_t prev_end =
                range_get_end_assert(mm_get_usable_list()[index - 1].range);

            if (prev_end != memmap->range.front) {
                index++;
                break;
            }

            mm_usable_list[index - 1].range.size += memmap->range.size;

            // Remove the current memmap.
            memmove(mm_usable_list + index,
                    mm_usable_list + index + 1,
                    (mm_usable_count - (index + 1)) * sizeof(struct mm_memmap));

            mm_usable_count--;
            if (index == mm_usable_count) {
                return;
            }
        } while (true);
    }
}