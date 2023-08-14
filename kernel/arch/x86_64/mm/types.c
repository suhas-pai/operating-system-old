/*
 * kernel/arch/x86_64/mm/types.c
 * © suhas pai
 */

#include "mm/page.h"
#include "limine.h"

const uint64_t PAGE_OFFSET = 0xffffc00000000000;
const uint64_t VMAP_BASE = 0xffffd00000000000;
const uint64_t VMAP_END = 0xffffe00000000000;

uint64_t MMIO_BASE = 0;
uint64_t MMIO_END = 0;

extern struct limine_paging_mode_request paging_mode_request;

uint8_t pgt_get_top_level() {
    const bool has_5lvl_paging =
        paging_mode_request.response->mode == LIMINE_PAGING_MODE_X86_64_5LVL;

    return has_5lvl_paging ? 5 : 4;
}

bool pte_is_present(const pte_t pte) {
    return (pte & __PTE_PRESENT) != 0;
}

bool pte_level_can_have_large(const uint8_t level) {
    return (level == 2 || level == 3);
}

bool pte_is_large(const pte_t pte) {
    return (pte & __PTE_LARGE) != 0;
}

uint64_t pte_get_addr(const pte_t pte) {
    return pte & PTE_PHYS_MASK;
}