/*
 * kernel/arch/aarch64/mm/types.c
 * © suhas pai
 */

#include <stdbool.h>

#include "limine.h"
#include "types.h"

const uint64_t PAGE_OFFSET = 0xffffc00000000000;
const uint64_t VMAP_BASE = 0xffffd00000000000;
const uint64_t VMAP_END = 0xffffe00000000000;

uint64_t MMIO_BASE = 0;
uint64_t MMIO_END = 0;
uint64_t PAGING_MODE = 0;

uint8_t pgt_get_top_level() {
    const bool has_5lvl_paging = PAGING_MODE == LIMINE_PAGING_MODE_AARCH64_5LVL;
    return has_5lvl_paging ? 5 : 4;
}

bool pte_is_present(const pte_t pte) {
    return (pte & __PTE_VALID) != 0;
}

bool pte_level_can_have_large(const uint8_t level) {
    return (level == 2 || level == 3 || level == 4);
}

bool pte_is_large(const pte_t pte) {
    return (pte & (__PTE_VALID | __PTE_TABLE)) == __PTE_VALID;
}

uint64_t pte_get_addr(const pte_t pte) {
    return pte & PTE_PHYS_MASK;
}
