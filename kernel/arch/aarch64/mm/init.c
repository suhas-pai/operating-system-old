/*
 * kernel/arch/aarch64/mm/init.c
 * © suhas pai
 */

#include "lib/size.h"
#include "mm/mm_types.h"

#include "limine.h"

static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0,
    .response = NULL
};

__unused static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0,
    .response = NULL
};

volatile struct limine_paging_mode_request paging_mode_request = {
    .id = LIMINE_PAGING_MODE_REQUEST,
    .revision = 0,
    .response = NULL
};

extern uint64_t HHDM_OFFSET;

void mm_init() {
    assert(hhdm_request.response != NULL);
    assert(memmap_request.response != NULL);
    assert(paging_mode_request.response != NULL);

    HHDM_OFFSET = hhdm_request.response->offset;

    MMIO_BASE = HHDM_OFFSET + kib(16);
    MMIO_END = MMIO_BASE + gib(4);
}