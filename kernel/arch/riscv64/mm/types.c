/*
 * kernel/arch/riscv64/mm/types.c
 * © suhas pai
 */

#include "lib/assert.h"
#include "mm/mm_types.h"

#include "limine.h"
#include "types.h"

const uint64_t PAGE_OFFSET = 0xffffffd000000000;
const uint64_t VMAP_BASE = 0xffffffe000000000;
const uint64_t VMAP_END = 0xfffffff000000000;

uint64_t PAGING_MODE = 0;
uint64_t MMIO_BASE = 0xffffffe000000000;
uint64_t MMIO_END = 0;

pgt_level_t pgt_get_top_level() {
    switch (PAGING_MODE) {
        case LIMINE_PAGING_MODE_RISCV_SV39:
            return 3;
        case LIMINE_PAGING_MODE_RISCV_SV48:
            return 4;
        case LIMINE_PAGING_MODE_RISCV_SV57:
            return 5;
    }

    verify_not_reached();
}

bool pte_is_present(const pte_t pte) {
    return (pte & __PTE_VALID) != 0;
}

bool pte_level_can_have_large(const pgt_level_t level) {
    return level > 1;
}

bool pte_is_large(const pte_t pte) {
    return ((pte & (__PTE_VALID | __PTE_READ | __PTE_WRITE | __PTE_EXEC))
                > __PTE_VALID);
}

uint64_t pte_get_phys_addr(const pte_t pte) {
    return (pte & PTE_PHYS_MASK) << 2;
}

