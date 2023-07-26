/*
 * kernel/arch/x86_64/mm/hhdm.c
 * © suhas pai
 */

#include "lib/assert.h"
#include "lib/overflow.h"

#include "limine.h"

uint64_t HHDM_OFFSET = 0;

void *phys_to_virt(const uint64_t phys) {
    return (void *)check_add_assert(HHDM_OFFSET, phys);
}

uint64_t virt_to_phys(const void *const virt) {
    assert((uint64_t)virt >= HHDM_OFFSET);
    return (uint64_t)virt - HHDM_OFFSET;
}
