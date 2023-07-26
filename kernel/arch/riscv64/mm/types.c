/*
 * kernel/arch/riscv64/mm/types.c
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

extern struct limine_paging_mode_request paging_mode_request;

uint8_t pgt_get_top_level() {
    const bool has_5lvl_paging =
        paging_mode_request.response->mode == LIMINE_PAGING_MODE_RISCV_SV57;

    return has_5lvl_paging ? 5 : 4;
}

bool pte_is_present(const pte_t pte) {
    return (pte & __PTE_VALID) != 0;
}

bool pte_is_large(const pte_t pte, const uint8_t level) {
    (void)pte;
    (void)level;
    return false;
}

