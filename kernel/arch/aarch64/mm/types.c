/*
 * kernel/arch/aarch64/mm/types.c
 * © suhas pai
 */

#include <stdbool.h>

#include "limine.h"
#include "types.h"

const uint64_t PAGE_OFFSET = 0xffffc00000000000;
extern struct limine_paging_mode_request paging_mode_request;

uint8_t pgt_get_top_level() {
    const bool has_5lvl_paging =
        paging_mode_request.response->mode == LIMINE_PAGING_MODE_AARCH64_5LVL;

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

