/*
 * kernek/boot.c
 * © suhas pai
 */

#if defined(__riscv) && defined(__LP64__)
    #include "dev/printk.h"
#endif /* defined(__riscv) && defined(__LP64__) */

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

static struct mm_memmap mm_memmap_list[64] = {0};
static uint64_t mm_memmap_count = 0;
static void *rsdp = NULL;

const struct mm_memmap *mm_get_memmap_list() {
    return mm_memmap_list;
}

uint64_t mm_get_memmap_count() {
    return mm_memmap_count;
}

void *boot_get_rsdp() {
    return rsdp;
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

    uint64_t memmap_index = 0;
    for (__auto_type memmap_iter = entries; memmap_iter != end; memmap_iter++) {
        if (memmap_index == countof(mm_memmap_list)) {
            panic("boot: too many memmaps\n");
        }

        const struct limine_memmap_entry *const memmap = *memmap_iter;
        mm_memmap_list[memmap_index] = (struct mm_memmap){
            .range = range_create(memmap->base, memmap->length),
            .kind = (enum mm_memmap_kind)(memmap->type + 1)
        };

        memmap_index++;
    }

    mm_memmap_count = memmap_index;

    if (rsdp_request.response == NULL ||
        rsdp_request.response->address == NULL)
    {
    #if !(defined(__riscv) && defined(__LP64__))
        panic("boot: acpi not found\n");
    #else
        printk(LOGLEVEL_WARN, "boot: acpi does not exist\n");
        return;
    #endif /* !(defined(__riscv) && defined(__LP64__)) */
    }

    rsdp = rsdp_request.response->address;
}