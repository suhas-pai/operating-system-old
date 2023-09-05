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
uint64_t FIRST_PAGE_PHYS = 0;

__optimize(3) pgt_level_t pgt_get_top_level() {
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

__optimize(3) bool pte_is_present(const pte_t pte) {
    return (pte & __PTE_VALID) != 0;
}

__optimize(3) bool pte_level_can_have_large(const pgt_level_t level) {
    return level > 1;
}

__optimize(3) bool pte_is_large(const pte_t pte) {
    return ((pte & (__PTE_VALID | __PTE_READ | __PTE_WRITE | __PTE_EXEC))
                > __PTE_VALID);
}

__optimize(3) pte_t pte_read(const pte_t *const pte) {
    return *(volatile const pte_t *)pte;
}

__optimize(3) void pte_write(pte_t *const pte, const pte_t value) {
    *(volatile pte_t *)pte = value;
}

__optimize(3) bool
pte_flags_equal(const pte_t pte, const pgt_level_t level, const uint64_t flags)
{
    (void)level;
    const uint64_t mask =
        __PTE_VALID | __PTE_READ | __PTE_WRITE | __PTE_EXEC | __PTE_USER |
        __PTE_GLOBAL;

    return (pte & mask) == flags;
}